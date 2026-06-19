#include "Mesh.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <tuple>

#include "mle/core/Assert.h"
#include "mle/math/Types2D.h"
#include "mle/math/Utils.h"
#include "mle/renderer/Buffer.h"
#include "mle/renderer/Primitive.h"

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

void buildEvaluationOrder(const std::vector<Mesh::Node>& nodes, std::vector<usize>& out_evaluation_order) {
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

// NOLINTNEXTLINE(misc-no-recursion) cool recursion
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

void markNodeAndAncestors(const std::vector<int>& node_parent, usize node_index, std::vector<bool>& out_included) {
    MLE_ASSERT_LOG(node_index < out_included.size(), "Node index out of range");
    out_included[node_index] = true;

    int parent = node_parent[node_index];
    while (parent >= 0) {
        const auto parent_index = as<usize>(parent);
        MLE_ASSERT_LOG(parent_index < out_included.size(), "Invalid parent node index");
        out_included[parent_index] = true;
        parent = node_parent[parent_index];
    }
}

void markSkinJointAncestors(const tinygltf::Model& model, const std::vector<int>& node_parent, int skin_index, std::vector<bool>& out_included) {
    if (skin_index < 0) {
        return;
    }

    MLE_ASSERT_LOG(skin_index < as<int>(model.skins.size()), "Invalid skin index in node");
    const auto& skin = model.skins[as<usize>(skin_index)];
    for (int joint_node : skin.joints) {
        MLE_ASSERT_LOG(joint_node >= 0 && joint_node < as<int>(model.nodes.size()), "Invalid skin joint node index");
        markNodeAndAncestors(node_parent, as<usize>(joint_node), out_included);
    }
}

vec2f projectTo2D(const vec3f& p, Axis axis) {
    switch (axis) {
        case Axis::X:
            return {p.y, p.z};
        case Axis::Y:
            return {p.x, p.z};
        case Axis::Z:
        default:
            return {p.x, p.y};
    }
}

f32 cross2d(const vec2f& O, const vec2f& A, const vec2f& B) {
    return ((A.x - O.x) * (B.y - O.y)) - ((A.y - O.y) * (B.x - O.x));
}

Polygon2f convexHull(std::vector<vec2f> pts) {
    if (pts.size() < 2) {
        return Polygon2f(std::move(pts));
    }

    std::sort(pts.begin(), pts.end(), [](const vec2f& a, const vec2f& b) { return a.x < b.x || (a.x == b.x && a.y < b.y); });
    auto last = std::unique(pts.begin(), pts.end(), [](const vec2f& a, const vec2f& b) { return mle::feq(a.x, b.x) && mle::feq(a.y, b.y); });
    pts.erase(last, pts.end());
    if (pts.size() < 3) {
        return Polygon2f(std::move(pts));
    }

    const usize n = pts.size();
    std::vector<vec2f> hull;
    hull.reserve(2 * n);

    // Lower hull
    for (usize i = 0; i < n; ++i) {
        while (hull.size() >= 2 && cross2d(hull[hull.size() - 2], hull[hull.size() - 1], pts[i]) <= 0.0F) {
            hull.pop_back();
        }
        hull.push_back(pts[i]);
    }

    // Upper hull
    const usize lower_size = hull.size();
    for (usize i = n - 2;; --i) {
        while (hull.size() > lower_size && cross2d(hull[hull.size() - 2], hull[hull.size() - 1], pts[i]) <= 0.0F) {
            hull.pop_back();
        }
        hull.push_back(pts[i]);
        if (i == 0) {
            break;
        }
    }

    hull.pop_back();
    return Polygon2f(std::move(hull));
}
}  // namespace

Mesh::~Mesh() = default;

void Mesh::init(const GLTF& gltf) {
    init(gltf, max<usize>());
}

void Mesh::init(const GLTF& gltf, usize root_node) {
    const auto& model = gltf.model();

    nodes_.clear();
    primitives_.clear();
    skins_.clear();
    evaluation_order_.clear();

    const usize node_count = model.nodes.size();
    nodes_.resize(node_count);

    std::vector<int> node_parent;
    buildNodeParents(model, node_parent);

    const bool has_root_node = root_node != max<usize>();
    MLE_ASSERT_LOG(!has_root_node || root_node < node_count, "Root node index out of range");
    const bool single_mesh_root = has_root_node && model.nodes[root_node].mesh >= 0;

    std::vector<bool> included(node_count, true);
    if (root_node != max<usize>()) {
        included.assign(node_count, false);
        if (single_mesh_root) {
            markNodeAndAncestors(node_parent, root_node, included);
            markSkinJointAncestors(model, node_parent, model.nodes[root_node].skin, included);
        } else {
            markNodeSubtree(model, root_node, included);
        }
    }

    for (usize nid = 0; nid < node_count; ++nid) {
        const tinygltf::Node& src_node = model.nodes[nid];

        Node& node = nodes_[nid];
        node.parent = (nid < node_parent.size()) ? node_parent[nid] : -1;
        if (!included[nid] || (has_root_node && !single_mesh_root && nid == root_node) || (node.parent >= 0 && !included[as<usize>(node.parent)])) {
            node.parent = -1;
        }
        node.name = src_node.name;
        node.included = included[nid];

        auto [t, r, s] = getNodeLocalTRS(src_node);
        node.base_translation = t;
        node.base_rotation = r;
        node.base_scale = s;

        const bool load_node_primitives = included[nid] && src_node.mesh >= 0 && (!single_mesh_root || nid == root_node);
        if (load_node_primitives) {
            MLE_ASSERT_LOG(src_node.mesh < as<int>(model.meshes.size()), "Invalid mesh index in node");
            const auto& mesh = model.meshes[as<usize>(src_node.mesh)];
            for (usize primitive_idx = 0; primitive_idx < mesh.primitives.size(); ++primitive_idx) {
                NodePrimitive np{};
                np.node_index = nid;
                np.skin_index = src_node.skin;
                np.primitive.load(gltf, as<usize>(src_node.mesh), primitive_idx);
                primitives_.push_back(std::move(np));
            }
        }
    }

    skins_.resize(model.skins.size());
    for (usize i = 0; i < model.skins.size(); ++i) {
        skins_[i].loadFromGLTF(gltf, i);
    }

    buildEvaluationOrder(nodes_, evaluation_order_);
}

void Mesh::evaluateBase(std::span<mat4f> out_node_globals) const {
    const usize node_count = nodes_.size();
    MLE_ASSERT_LOG(out_node_globals.size() >= node_count, "out_node_globals span too small");

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

usize Mesh::getNodeIdxByName(const std::string& name) const {
    for (usize i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i].included && nodes_[i].name == name) {
            return i;
        }
    }
    return max<usize>();
}

std::optional<const Primitive*> Mesh::getPrimitiveByName(const std::string& name) const {
    const usize node_idx = getNodeIdxByName(name);
    if (node_idx == max<usize>()) {
        return std::nullopt;
    }

    for (const auto& node_primitive : primitives_) {
        if (node_primitive.node_index == node_idx) {
            return &node_primitive.primitive;
        }
    }

    return std::nullopt;
}

Polygon2f Mesh::projectPolygon(Axis axis, f32 /*offset*/) const {
    usize total_verts = 0;
    for (const auto& np : primitives_) {
        total_verts += np.primitive.getCpuPositions().size();
    }
    if (total_verts == 0) {
        return {};
    }

    const usize node_count = nodes_.size();
    std::vector<mat4f> node_globals(node_count);
    evaluateBase(std::span<mat4f>(node_globals));

    std::vector<vec2f> all_pts;
    all_pts.reserve(total_verts);
    for (const auto& np : primitives_) {
        mat4f xform = node_globals[np.node_index];
        if (np.skin_index >= 0 && np.skin_index < as<int>(skins_.size())) {
            const auto& skin = skins_[np.skin_index];
            if (skin.jointCount() > 0) {
                const auto& joint = skin.getJoints()[0];
                xform = node_globals[joint.node_index] * joint.inverse_bind;
            }
        }

        for (const vec3f& local_pos : np.primitive.getCpuPositions()) {
            const vec3f world_pos = vec3f(xform * vec4f(local_pos, 1.0F));
            all_pts.push_back(projectTo2D(world_pos, axis));
        }
    }

    return convexHull(std::move(all_pts));
}

}  // namespace mle
