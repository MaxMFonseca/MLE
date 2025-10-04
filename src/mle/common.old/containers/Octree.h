/**
 * @file Octree.h
 * @brief Generic pointerless linear octree for spatial partitioning.
 *
 * This header defines a flat-memory octree structure that stores spatial data in 3D.
 * Each node is stored in a contiguous array and indexed by offset arithmetic rather than pointers.
 *
 * Features:
 * - Dynamic insertion of leaf nodes using a flat `std::vector<Node>`.
 * - Optional ray intersection support (`Ray3f`) returning the first intersected leaf.
 * - Implicit internal node hierarchy using `children_size` arrays per node.
 * - Optional leaf removal and spatial queries for parent nodes and contents.
 */

#pragma once

#include <array>
#include <list>
#include <vector>

#include "mle/common/Types.h"
#include "mle/common/math/Types3D.h"

// TODO: this could use some love
namespace mle {
template <typename NodeDataT>
class Octree {
  public:
    struct PosData {
        vec3u pos;
        NodeDataT data;
    };

    struct IntersectionResult {
        vec3i node;
        f32 near, far;
    };

  private:
    /**
     * @brief Internal node in the octree.
     *
     * Represents either an internal branch (with `children_size`) or a terminal leaf node (with `Leaf` data).
     * The structure uses a union for compact storage — at runtime, a node is either:
     * - a branch: with 0 or more populated child offsets in `children_size`
     * - a leaf: with valid user data in `Leaf`
     *
     * @note Lifetime of union members must be managed carefully due to non-trivial types.
     */
    struct Node {
        /// Payload stored in leaf nodes.
        struct Leaf {
            NodeDataT data;  ///< The user-defined leaf payload.
            bool valid;      ///< Whether the data is valid (active).
        };

        union {
            std::array<u64, 8> children_size{0};  ///< Number of nodes in each child octant subtree.
            Leaf leaf;                            ///< Leaf payload (used if node is terminal).
        };
    };

  public:
    Octree(const Octree&) = delete;
    Octree(Octree&&) = default;
    Octree& operator=(const Octree&) = delete;
    Octree& operator=(Octree&&) = default;

    explicit Octree(vec3u size) :
        size_(size) {
        nodes_.resize(1);
    }
    ~Octree() = default;

    void setNode(vec3u pos, NodeDataT data);
    std::optional<NodeDataT> getNode(vec3u pos);
    void removeNode(vec3u pos);

    std::vector<PosData> getNodes();
    std::vector<Boxf> getParents();

    std::optional<IntersectionResult> intersect(Ray3f ray);

    auto size() const { return size_; }

    u32 getNodeIdx(vec3u pos);

    // TODO: void cleanup(); remove invalid nodes and parents

  private:
    struct IdxElemAdded {
        u64 idx = 0;
        u64 elements_added = 0;
    };
    IdxElemAdded getNodeIdxInternal(vec3u pos, vec3u curr_size = {0, 0, 0}, vec3u curr_pos = {0, 0, 0}, u64 current_idx = 0);

    void fillBranchData(std::vector<PosData>& ret, u64 idx, vec3u curr_pos, vec3u curr_size);
    void fillBranchParents(std::vector<Boxf>& ret, u64 idx, vec3u curr_pos, vec3u curr_size);

    std::optional<IntersectionResult> intersectInternal(Ray3f ray, u64 idx, vec3u pos, vec3u size);

  private:
    std::vector<Node> nodes_;
    vec3u size_{};
    u32 data_count_ = 0;
};

template <typename NodeDataT>
void Octree<NodeDataT>::setNode(vec3u pos, NodeDataT data) {
    auto idx = getNodeIdx(pos);
    nodes_[idx].leaf = {true, std::move(data)};
}

template <typename NodeDataT>
std::optional<NodeDataT> Octree<NodeDataT>::getNode(vec3u pos) {
    auto idx = getNodeIdx(pos);
    if (!nodes_[idx].leaf.valid) {
        return std::nullopt;
    }
    return nodes_[idx].leaf.data;
}

template <typename NodeDataT>
void Octree<NodeDataT>::removeNode(vec3u pos) {
    auto idx = getNodeIdx(pos);
    nodes_[idx].leaf.valid = false;
}

template <typename NodeDataT>
std::vector<typename Octree<NodeDataT>::PosData> Octree<NodeDataT>::getNodes() {
    std::vector<PosData> ret;
    ret.reserve(data_count_);
    fillBranchData(ret, 0, {0, 0, 0}, size_);
    return ret;
}

template <typename NodeDataT>
std::vector<Boxf> Octree<NodeDataT>::getParents() {
    std::vector<Boxf> ret;
    ret.reserve(nodes_.size() - data_count_);
    fillBranchParents(ret, 0, {0, 0, 0}, size_);
    return ret;
}

template <typename NodeDataT>
u32 Octree<NodeDataT>::getNodeIdx(vec3u pos) {
    return getNodeIdxInternal(pos).idx;
}

template <typename NodeDataT>
Octree<NodeDataT>::IdxElemAdded Octree<NodeDataT>::getNodeIdxInternal(vec3u pos, vec3u curr_size, vec3u curr_pos, u64 current_idx) {
    if (current_idx == 0) {
        curr_size = size_;
        assert(pos.x < curr_size.x && pos.y < curr_size.y && pos.z < curr_size.z);
    }

    u32 is_right = static_cast<u32>((curr_size.x > 1) && (pos.x >= (curr_pos.x + curr_size.x / 2)));
    u32 is_top = static_cast<u32>((curr_size.y > 1) && (pos.y >= (curr_pos.y + curr_size.y / 2)));
    u32 is_back = static_cast<u32>((curr_size.z > 1) && (pos.z >= (curr_pos.z + curr_size.z / 2)));

    u32 octant_idx = is_right;
    octant_idx |= is_top << 1U;
    octant_idx |= is_back << 2U;

    vec3u next_size = curr_size / 2U;
    next_size.x = std::max(1U, next_size.x);
    next_size.y = std::max(1U, next_size.y);
    next_size.z = std::max(1U, next_size.z);
    next_size.x += is_right * (curr_size.x % 2);
    next_size.y += is_top * (curr_size.y % 2);
    next_size.z += is_back * (curr_size.z % 2);

    vec3u child_pos = curr_pos;
    child_pos.x += is_right * curr_size.x / 2;
    child_pos.y += is_top * curr_size.y / 2;
    child_pos.z += is_back * curr_size.z / 2;

    u64 octant_advance = 1;
    for (u64 i = 0; i < octant_idx; ++i) {
        octant_advance += nodes_.at(current_idx).children_size.at(i);
    }

    bool added = false;
    if (nodes_.at(current_idx).children_size.at(octant_idx) == 0) {
        nodes_.insert(nodes_.begin() + current_idx + octant_advance, 1, {});
        added = true;
    }

    bool next_is_leaf = next_size.x == 1 && next_size.y == 1 && next_size.z == 1;
    if (next_is_leaf) {
        nodes_.at(current_idx).children_size.at(octant_idx) = 1;
        if (added) {
            data_count_++;
        }
        return {current_idx + octant_advance, added};
    }

    auto get_pos_result = getNodeIdxInternal(pos, next_size, child_pos, current_idx + octant_advance);

    get_pos_result.elements_added += added;
    nodes_.at(current_idx).children_size.at(octant_idx) += get_pos_result.elements_added;

    return get_pos_result;
}

template <typename NodeDataT>
void Octree<NodeDataT>::fillBranchData(std::vector<PosData>& ret, u64 idx, vec3u curr_pos, vec3u curr_size) {
    auto& node = nodes_.at(idx);

    u64 octant_advance = 1;
    for (u64 octant = 0; octant < 8; ++octant) {
        if (node.children_size.at(octant) == 0) {
            continue;
        }

        u32 is_right = static_cast<u32>(static_cast<bool>(octant & 1U));
        u32 is_top = static_cast<u32>(static_cast<bool>(octant & 2U));
        u32 is_back = static_cast<u32>(static_cast<bool>(octant & 4U));

        if (node.children_size.at(octant) == 1) {
            vec3u add_pos = {is_right, is_top, is_back};
            ret.push_back({curr_pos + add_pos, nodes_.at(idx + octant_advance).leaf.data});
            octant_advance += node.children_size.at(octant);
            continue;
        }

        vec3u next_size = curr_size / 2U;
        next_size.x = std::max(1U, next_size.x);
        next_size.y = std::max(1U, next_size.y);
        next_size.z = std::max(1U, next_size.z);
        next_size.x += is_right * (curr_size.x % 2);
        next_size.y += is_top * (curr_size.y % 2);
        next_size.z += is_back * (curr_size.z % 2);

        vec3u child_pos = curr_pos;
        child_pos.x += is_right * curr_size.x / 2;
        child_pos.y += is_top * curr_size.y / 2;
        child_pos.z += is_back * curr_size.z / 2;

        fillBranchData(ret, idx + octant_advance, child_pos, next_size);

        octant_advance += node.children_size.at(octant);
    }
}

template <typename NodeDataT>
void Octree<NodeDataT>::fillBranchParents(std::vector<Boxf>& ret, u64 idx, vec3u curr_pos, vec3u curr_size) {
    auto& node = nodes_.at(idx);

    ret.emplace_back(curr_pos, curr_size);

    u64 octant_advance = 1;
    for (u64 octant = 0; octant < 8; ++octant) {
        if (node.children_size.at(octant) == 0 || node.children_size.at(octant) == 1) {
            continue;
        }

        u32 is_right = static_cast<u32>(static_cast<bool>(octant & 1U));
        u32 is_top = static_cast<u32>(static_cast<bool>(octant & 2U));
        u32 is_back = static_cast<u32>(static_cast<bool>(octant & 4U));

        vec3u next_size = curr_size / 2U;
        next_size.x = std::max(1U, next_size.x);
        next_size.y = std::max(1U, next_size.y);
        next_size.z = std::max(1U, next_size.z);
        next_size.x += is_right * (curr_size.x % 2);
        next_size.y += is_top * (curr_size.y % 2);
        next_size.z += is_back * (curr_size.z % 2);

        vec3u child_pos = curr_pos;
        child_pos.x += is_right * curr_size.x / 2;
        child_pos.y += is_top * curr_size.y / 2;
        child_pos.z += is_back * curr_size.z / 2;

        fillBranchParents(ret, idx + octant_advance, child_pos, next_size);

        octant_advance += node.children_size.at(octant);
    }
}

template <typename NodeDataT>
std::optional<typename Octree<NodeDataT>::IntersectionResult> Octree<NodeDataT>::intersect(Ray3f ray) {
    return intersectInternal(ray, 0, {0, 0, 0}, size_);
}

template <typename NodeDataT>
std::optional<typename Octree<NodeDataT>::IntersectionResult> Octree<NodeDataT>::intersectInternal(Ray3f ray, u64 idx, vec3u pos, vec3u size) {
    struct IntHelper {
        u64 advance;
        u64 octant;
        vec3u pos;
        vec3u size;
        f32 near;
        f32 far;
    };

    auto& node = nodes_.at(idx);

    std::list<IntHelper> intersections;

    u64 octant_advance = 1;
    for (u64 octant = 0; octant < 8; ++octant) {
        if (node.children_size.at(octant) == 0) {
            continue;
        }

        u32 is_right = static_cast<u32>(static_cast<bool>(octant & 1U));
        u32 is_top = static_cast<u32>(static_cast<bool>(octant & 2U));
        u32 is_back = static_cast<u32>(static_cast<bool>(octant & 4U));

        vec3u octant_size = size / 2U;
        octant_size.x = std::max(1U, octant_size.x);
        octant_size.y = std::max(1U, octant_size.y);
        octant_size.z = std::max(1U, octant_size.z);
        octant_size.x += is_right * (size.x % 2);
        octant_size.y += is_top * (size.y % 2);
        octant_size.z += is_back * (size.z % 2);

        vec3u octant_pos = pos;
        octant_pos.x += is_right * size.x / 2;
        octant_pos.y += is_top * size.y / 2;
        octant_pos.z += is_back * size.z / 2;

        auto octant_box = Boxf{octant_pos, octant_size};
        auto intersection = ray.intersect(octant_box);
        if (!intersection) {
            octant_advance += node.children_size.at(octant);
            continue;
        }

        auto it = intersections.begin();
        while (it != intersections.end() && it->near < intersection->near) {
            ++it;
        }
        intersections.insert(it, IntHelper{octant_advance, octant, octant_pos, octant_size, intersection->near, intersection->far});
        octant_advance += node.children_size.at(octant);
    }

    for (const auto& intersection : intersections) {
        if (node.children_size.at(intersection.octant) == 1) {
            return IntersectionResult{intersection.pos, intersection.near, intersection.far};
        }

        auto result = intersectInternal(ray, idx + intersection.advance, intersection.pos, intersection.size);
        if (result) {
            return result;
        }
    }
    return std::nullopt;
}
}  // namespace mle
