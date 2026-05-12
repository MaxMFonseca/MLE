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

void computeNodeGlobalRecursive(usize node_index, const std::vector<Model::Node>& nodes, const std::vector<mat4f>& local_mats, std::vector<mat4f>& global_mats,
                                std::vector<bool>& visited) {
    if (visited[node_index]) {
        return;
    }

    const auto& node = nodes[node_index];
    if (node.parent >= 0) {
        const auto parent_idx = as<usize>(node.parent);
        computeNodeGlobalRecursive(parent_idx, nodes, local_mats, global_mats, visited);
        global_mats[node_index] = global_mats[parent_idx] * local_mats[node_index];
    } else {
        global_mats[node_index] = local_mats[node_index];
    }

    visited[node_index] = true;
}
}  // namespace

Model::~Model() = default;

void Model::init(const GLTF& gltf) {
    const auto& model = gltf.model();

    nodes_.clear();
    meshes_.clear();

    const usize node_count = model.nodes.size();
    nodes_.resize(node_count);

    std::vector<int> node_parent;
    buildNodeParents(model, node_parent);

    for (usize nid = 0; nid < node_count; ++nid) {
        const tinygltf::Node& src_node = model.nodes[nid];

        Node& node = nodes_[nid];
        node.parent = (nid < node_parent.size()) ? node_parent[nid] : -1;
        node.name = src_node.name;

        auto [t, r, s] = getNodeLocalTRS(src_node);
        node.base_translation = t;
        node.base_rotation = r;
        node.base_scale = s;

        if (src_node.mesh >= 0) {
            MLE_ASSERT_LOG(src_node.mesh < as<int>(model.meshes.size()), "Invalid mesh index in node");
            NodeMesh nm{};
            nm.node_index = nid;
            nm.mesh.load(gltf, as<usize>(src_node.mesh));
            meshes_.push_back(std::move(nm));
        }
    }
}

void Model::evaluateBase(std::vector<mat4f>& out_node_globals) const {
    const usize node_count = nodes_.size();

    out_node_globals.resize(node_count);

    std::vector<mat4f> local_mats(node_count);
    std::vector<mat4f> global_mats(node_count);
    std::vector<bool> visited(node_count, false);

    for (usize nid = 0; nid < node_count; ++nid) {
        const Node& node = nodes_[nid];
        local_mats[nid] = makeTRS(node.base_translation, node.base_rotation, node.base_scale);
    }

    for (usize nid = 0; nid < node_count; ++nid) {
        if (!visited[nid]) {
            computeNodeGlobalRecursive(nid, nodes_, local_mats, global_mats, visited);
        }
    }

    out_node_globals = global_mats;
}

usize Model::getNodeIdxByName(const std::string& name) const {
    for (usize i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i].name == name) {
            return i;
        }
    }
    return max<usize>();
}

}  // namespace mle
