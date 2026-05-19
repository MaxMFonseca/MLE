#include "Model.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <tuple>

#include "mle/core/Assert.h"
#include "mle/renderer/Buffer.h"

namespace mle {
namespace {
void buildNodeParents(const tinygltf::Model& model, std::vector<int>& out_node_parent) {
    out_node_parent.assign(model.nodes.size(), -1);
    for (int nid = 0; nid < as<int>(model.nodes.size()); ++nid) {
        const auto& node = model.nodes[as<usize>(nid)];
        for (int child : node.children) {
            MLE_ASSERT_LOG(child >= 0 && child < as<int>(model.nodes.size()), "Invalid child node index");
            out_node_parent[as<usize>(child)] = nid;
        }
    }
}

void buildEvaluationOrder(const std::vector<Model::Node>& nodes, std::vector<usize>& out_evaluation_order) {
    out_evaluation_order.clear();
    const usize node_count = nodes.size();
    out_evaluation_order.reserve(node_count);

    std::vector<bool> added(node_count, false);

    while (out_evaluation_order.size() < node_count) {
        bool changed = false;
        for (usize i = 0; i < node_count; ++i) {
            if (added[i]) {
                continue;
            }

            if (nodes[i].parent < 0 || added[as<usize>(nodes[i].parent)]) {
                out_evaluation_order.push_back(i);
                added[i] = true;
                changed = true;
            }
        }
        MLE_ASSERT_LOG(changed, "Circular dependency detected in node hierarchy or disconnected nodes");
    }
}

std::tuple<vec3f, quat, vec3f> getNodeLocalTRS(const tinygltf::Node& node) {
    vec3f t{};
    quat r{};
    vec3f s{};

    if (node.matrix.size() == 16) {
        mat4f m{};
        for (int i = 0; i < 16; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) safe
            glm::value_ptr(m)[i] = as<float>(node.matrix[as<usize>(i)]);
        }

        vec3f skew;
        vec4f perspective;

        bool ok = glm::decompose(m, s, r, t, skew, perspective);
        MLE_ASSERT_LOG(ok, "Failed to decompose node.matrix");
        r = glm::normalize(r);
    }

    if (!node.translation.empty()) {
        t = vec3f{
            as<f32>(node.translation[0]),
            as<f32>(node.translation[1]),
            as<f32>(node.translation[2]),
        };
    } else {
        t = vec3f{0.0F, 0.0F, 0.0F};
    }

    if (!node.rotation.empty()) {
        r = quat{
            as<f32>(node.rotation[3]),
            as<f32>(node.rotation[0]),
            as<f32>(node.rotation[1]),
            as<f32>(node.rotation[2]),
        };
        r = glm::normalize(r);
    } else {
        r = quat{1.0F, 0.0F, 0.0F, 0.0F};
    }

    if (!node.scale.empty()) {
        s = vec3f{
            as<f32>(node.scale[0]),
            as<f32>(node.scale[1]),
            as<f32>(node.scale[2]),
        };
    } else {
        s = vec3f{1.0F, 1.0F, 1.0F};
    }

    return std::make_tuple(t, r, s);
}

mat4f makeTRS(const vec3f& t, const quat& r, const vec3f& s) {
    mat4f f_t = glm::translate(mat4f{1.0F}, t);
    mat4f f_r = glm::mat4_cast(r);
    mat4f f_s = glm::scale(mat4f{1.0F}, s);
    return f_t * f_r * f_s;
}

void markNodeSubtree(const tinygltf::Model& model, usize node_index, std::vector<bool>& out_included) {
    MLE_ASSERT_LOG(node_index < model.nodes.size(), "Root node index out of range");
    if (out_included[node_index]) {
        return;
    }

    out_included[node_index] = true;
    const auto& node = model.nodes[node_index];
    for (int child : node.children) {
        MLE_ASSERT_LOG(child >= 0 && child < as<int>(model.nodes.size()), "Invalid child node index");
        markNodeSubtree(model, as<usize>(child), out_included);
    }
}
}  // namespace

Model::~Model() = default;

void Model::init(const GLTF& gltf) {
    init(gltf, max<usize>());
}

void Model::init(const GLTF& gltf, usize root_node) {
    const auto& model = gltf.model();

    nodes_.clear();
    meshes_.clear();
    evaluation_order_.clear();

    const usize node_count = model.nodes.size();
    nodes_.resize(node_count);

    std::vector<int> node_parent;
    buildNodeParents(model, node_parent);

    std::vector<bool> included(node_count, true);
    if (root_node != max<usize>()) {
        included.assign(node_count, false);
        markNodeSubtree(model, root_node, included);
    }

    for (usize nid = 0; nid < node_count; ++nid) {
        const tinygltf::Node& src_node = model.nodes[nid];

        Node& node = nodes_[nid];
        node.parent = (nid < node_parent.size()) ? node_parent[nid] : -1;
        if (!included[nid] || (root_node != max<usize>() && nid == root_node)) {
            node.parent = -1;
        }
        node.name = src_node.name;
        node.included = included[nid];

        auto [t, r, s] = getNodeLocalTRS(src_node);
        node.base_translation = t;
        node.base_rotation = r;
        node.base_scale = s;

        if (included[nid] && src_node.mesh >= 0) {
            MLE_ASSERT_LOG(src_node.mesh < as<int>(model.meshes.size()), "Invalid mesh index in node");
            const auto& mesh = model.meshes[as<usize>(src_node.mesh)];
            for (usize primitive_idx = 0; primitive_idx < mesh.primitives.size(); ++primitive_idx) {
                NodeMesh nm{};
                nm.node_index = nid;
                nm.skin_index = src_node.skin;
                nm.mesh.load(gltf, as<usize>(src_node.mesh), primitive_idx);
                meshes_.push_back(std::move(nm));
            }
        }
    }

    buildEvaluationOrder(nodes_, evaluation_order_);
}

void Model::evaluateBase(std::vector<mat4f>& out_node_globals) const {
    const usize node_count = nodes_.size();
    out_node_globals.resize(node_count);

    for (usize nid : evaluation_order_) {
        const Node& node = nodes_[nid];
        const mat4f local = makeTRS(node.base_translation, node.base_rotation, node.base_scale);

        if (node.parent >= 0) {
            out_node_globals[nid] = out_node_globals[as<usize>(node.parent)] * local;
        } else {
            out_node_globals[nid] = local;
        }
    }
}


usize Model::getNodeIdxByName(const std::string& name) const {
    for (usize i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i].included && nodes_[i].name == name) {
            return i;
        }
    }
    return max<usize>();
}

}  // namespace mle
