#pragma once

#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ModelTestAssets.h"
#include "mle/math/Types.h"
#include "mle/renderer/Types.h"

namespace mle::user {
struct ModelTestLoadedMesh {
    MeshRef resource = nullptr;
    bool skinned = false;
    std::vector<std::pair<std::string, usize>> included_nodes;
};

struct ModelTestMeshSelection {
    std::string id;
    std::string path;
    std::string selector;
    usize mesh_node_index = max<usize>();
    MeshRef resource = nullptr;
    std::vector<std::pair<std::string, usize>> included_nodes;
};

struct ModelTestAnimationSelection {
    std::string id;
    std::string path;
    std::string selector;
    AnimationClipRef resource = nullptr;
};

using ModelTestMeshLoader = std::function<std::expected<ModelTestLoadedMesh, std::string>(std::string_view, usize)>;
using ModelTestAnimationLoader = std::function<std::expected<AnimationClipRef, std::string>(std::string_view, std::string_view)>;

class ModelTestScene {
  public:
    ModelTestScene(ModelTestAssets assets, ModelTestMeshLoader mesh_loader = {}, ModelTestAnimationLoader animation_loader = {});

    std::expected<std::vector<std::string>, std::string> startup();
    CompletionResult complete(ModelResourceKind kind, std::string_view query);

    bool submitModel(std::string_view resource_id);
    bool submitHeldItem(std::string_view resource_id);
    bool submitAnimation(std::string_view resource_id);
    bool submitAttachment(std::string_view selector);
    void clearHeldItem();
    void clearAnimation();
    void clearAttachment();

    [[nodiscard]] const ModelTestMeshSelection* model() const;
    [[nodiscard]] const ModelTestMeshSelection* heldItem() const;
    [[nodiscard]] const ModelTestAnimationSelection* animation() const;
    [[nodiscard]] const std::string& attachmentNode() const { return attachment_node_; }
    [[nodiscard]] usize attachmentNodeIndex() const { return attachment_node_index_; }
    [[nodiscard]] const std::string& status() const { return status_; }

    void setHeldItemTranslation(vec3f translation) { held_item_translation_ = translation; }
    void setHeldItemRotation(vec3f rotation) { held_item_rotation_ = rotation; }
    void setHeldItemScale(f32 scale) { held_item_scale_ = scale; }
    [[nodiscard]] vec3f heldItemTranslation() const { return held_item_translation_; }
    [[nodiscard]] vec3f heldItemRotation() const { return held_item_rotation_; }
    [[nodiscard]] f32 heldItemScale() const { return held_item_scale_; }
    [[nodiscard]] mat4f heldItemTransform(const mat4f& attachment_global) const;

  private:
    struct ResolvedMesh {
        ModelResourceId id;
        std::string selector;
        usize node_index = max<usize>();
    };

    std::expected<ResolvedMesh, std::string> resolveMesh(std::string_view resource_id);
    bool fail(std::string message);

    ModelTestAssets assets_;
    ModelTestMeshLoader mesh_loader_;
    ModelTestAnimationLoader animation_loader_;
    std::optional<ModelTestMeshSelection> model_;
    std::optional<ModelTestMeshSelection> held_item_;
    std::optional<ModelTestAnimationSelection> animation_;
    std::string attachment_node_;
    usize attachment_node_index_ = max<usize>();
    std::string status_;
    vec3f held_item_translation_{};
    vec3f held_item_rotation_{};
    f32 held_item_scale_ = 1.0F;
};
}  // namespace mle::user
