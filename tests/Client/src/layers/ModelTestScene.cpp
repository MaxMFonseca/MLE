#include "ModelTestScene.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <iterator>
#include <ranges>

namespace mle::user {
namespace {
std::string ambiguityMessage(std::string_view kind, std::string_view selector, std::string_view path) {
    return std::string{kind} + " selector '" + std::string{selector} + "' is ambiguous in " + std::string{path};
}

std::string notFoundMessage(std::string_view kind, std::string_view selector, std::string_view path) {
    return std::string{kind} + " selector '" + std::string{selector} + "' was not found in " + std::string{path};
}

mat4f removeScale(const mat4f& transform) {
    mat4f result = transform;
    for (usize column = 0; column < 3; ++column) {
        const vec3f axis{result[column]};
        const f32 length = glm::length(axis);
        if (length > 1.0e-6F) {
            result[column] = vec4f{axis / length, 0.0F};
        }
    }
    return result;
}
}  // namespace

ModelTestScene::ModelTestScene(ModelTestAssets assets, ModelTestMeshLoader mesh_loader, ModelTestAnimationLoader animation_loader) :
    assets_(std::move(assets)),
    mesh_loader_(std::move(mesh_loader)),
    animation_loader_(std::move(animation_loader)) {
}

std::expected<std::vector<std::string>, std::string> ModelTestScene::startup() {
    auto paths = assets_.scanUserPaths();
    if (!paths.has_value()) {
        status_ = paths.error();
        return std::unexpected{paths.error()};
    }
    status_ = "Found " + std::to_string(paths->size()) + " user model resource(s)";
    return paths;
}

CompletionResult ModelTestScene::complete(ModelResourceKind kind, std::string_view query) {
    if (kind == ModelResourceKind::ATTACHMENT_NODE) {
        if (!model_.has_value()) {
            return CompletionResult{.replacement = std::string{query}, .suggestions = {}, .message = "Select a model before completing an attachment node"};
        }
        std::vector<std::string> node_names;
        node_names.reserve(model_->included_nodes.size());
        for (const auto& [name, index] : model_->included_nodes) {
            (void)index;
            node_names.push_back(name);
        }
        return assets_.complete(kind, query, node_names);
    }
    return assets_.complete(kind, query);
}

std::expected<ModelTestScene::ResolvedMesh, std::string> ModelTestScene::resolveMesh(std::string_view resource_id) {
    auto parsed = assets_.parseResourceId(resource_id);
    if (!parsed.has_value()) {
        return std::unexpected{parsed.error()};
    }

    auto metadata = assets_.metadataFor(parsed->path);
    if (!metadata.has_value()) {
        return std::unexpected{metadata.error()};
    }
    const auto& mesh_nodes = metadata->get().mesh_nodes;
    if (mesh_nodes.empty()) {
        return std::unexpected{"No mesh nodes were found in " + parsed->path};
    }

    if (!parsed->selector.has_value()) {
        if (mesh_nodes.size() != 1) {
            return std::unexpected{"Mesh selection is ambiguous in " + parsed->path + "; add an exact #selector"};
        }
        return ResolvedMesh{.id = std::move(*parsed), .selector = mesh_nodes.front().first, .node_index = mesh_nodes.front().second};
    }

    const std::string selector = *parsed->selector;
    std::vector<std::pair<std::string, usize>> matches;
    std::ranges::copy_if(mesh_nodes, std::back_inserter(matches), [&](const auto& mesh_node) { return mesh_node.first == selector; });
    if (matches.empty()) {
        return std::unexpected{notFoundMessage("Mesh", selector, parsed->path)};
    }
    if (matches.size() != 1) {
        return std::unexpected{ambiguityMessage("Mesh", selector, parsed->path)};
    }
    return ResolvedMesh{.id = std::move(*parsed), .selector = selector, .node_index = matches.front().second};
}

bool ModelTestScene::submitModel(std::string_view resource_id) {
    auto resolved = resolveMesh(resource_id);
    if (!resolved.has_value()) {
        return fail(resolved.error());
    }
    if (!mesh_loader_) {
        return fail("No mesh resource loader is configured");
    }
    auto loaded = mesh_loader_(resolved->id.path, resolved->node_index);
    if (!loaded.has_value()) {
        return fail(loaded.error());
    }
    if (loaded->resource == nullptr) {
        return fail("Model resource loader returned no mesh");
    }

    const std::string id = resolved->id.path + "#" + resolved->selector;
    model_ = ModelTestMeshSelection{.id = id,
                                    .path = resolved->id.path,
                                    .selector = resolved->selector,
                                    .mesh_node_index = resolved->node_index,
                                    .resource = loaded->resource,
                                    .included_nodes = std::move(loaded->included_nodes)};
    attachment_node_.clear();
    attachment_node_index_ = max<usize>();
    status_ = "Loaded model " + id;
    return true;
}

bool ModelTestScene::submitHeldItem(std::string_view resource_id) {
    auto resolved = resolveMesh(resource_id);
    if (!resolved.has_value()) {
        return fail(resolved.error());
    }
    if (!mesh_loader_) {
        return fail("No mesh resource loader is configured");
    }
    auto loaded = mesh_loader_(resolved->id.path, resolved->node_index);
    if (!loaded.has_value()) {
        return fail(loaded.error());
    }
    if (loaded->resource == nullptr) {
        return fail("Held-item resource loader returned no mesh");
    }
    if (loaded->skinned) {
        return fail("Held item must be non-skinned");
    }

    const std::string id = resolved->id.path + "#" + resolved->selector;
    held_item_ = ModelTestMeshSelection{.id = id,
                                        .path = resolved->id.path,
                                        .selector = resolved->selector,
                                        .mesh_node_index = resolved->node_index,
                                        .resource = loaded->resource,
                                        .included_nodes = std::move(loaded->included_nodes)};
    status_ = "Loaded held item " + id;
    return true;
}

bool ModelTestScene::submitAnimation(std::string_view resource_id) {
    auto parsed = assets_.parseResourceId(resource_id);
    if (!parsed.has_value()) {
        return fail(parsed.error());
    }
    if (!parsed->selector.has_value()) {
        return fail("Animation resource ID requires an exact #selector");
    }

    auto metadata = assets_.metadataFor(parsed->path);
    if (!metadata.has_value()) {
        return fail(metadata.error());
    }
    const std::string& selector = *parsed->selector;
    const auto count = std::ranges::count(metadata->get().animation_names, selector);
    if (count == 0) {
        return fail(notFoundMessage("Animation", selector, parsed->path));
    }
    if (count != 1) {
        return fail(ambiguityMessage("Animation", selector, parsed->path));
    }
    if (!animation_loader_) {
        return fail("No animation resource loader is configured");
    }
    auto loaded = animation_loader_(parsed->path, selector);
    if (!loaded.has_value()) {
        return fail(loaded.error());
    }
    if (*loaded == nullptr) {
        return fail("Animation resource loader returned no clip");
    }

    const std::string id = parsed->normalized();
    animation_ = ModelTestAnimationSelection{.id = id, .path = parsed->path, .selector = selector, .resource = *loaded};
    status_ = "Loaded animation " + id;
    return true;
}

bool ModelTestScene::submitAttachment(std::string_view selector) {
    if (!model_.has_value()) {
        return fail("Select a model before selecting an attachment node");
    }
    if (selector.empty()) {
        return fail("Attachment selector is empty");
    }
    auto matches = model_->included_nodes | std::views::filter([&](const auto& node) { return node.first == selector; });
    const auto first = matches.begin();
    if (first == matches.end()) {
        return fail(notFoundMessage("Attachment node", selector, model_->path));
    }
    if (std::next(first) != matches.end()) {
        return fail(ambiguityMessage("Attachment node", selector, model_->path));
    }

    attachment_node_ = selector;
    attachment_node_index_ = first->second;
    status_ = "Selected attachment node " + attachment_node_;
    return true;
}

void ModelTestScene::clearHeldItem() {
    held_item_.reset();
    status_ = "Held item cleared";
}

void ModelTestScene::clearAnimation() {
    animation_.reset();
    status_ = "Animation cleared";
}

void ModelTestScene::clearAttachment() {
    attachment_node_.clear();
    attachment_node_index_ = max<usize>();
    status_ = "Attachment node cleared";
}

const ModelTestMeshSelection* ModelTestScene::model() const {
    return model_.has_value() ? &*model_ : nullptr;
}

const ModelTestMeshSelection* ModelTestScene::heldItem() const {
    return held_item_.has_value() ? &*held_item_ : nullptr;
}

const ModelTestAnimationSelection* ModelTestScene::animation() const {
    return animation_.has_value() ? &*animation_ : nullptr;
}

mat4f ModelTestScene::heldItemTransform(const mat4f& attachment_global) const {
    const mat4f translation = glm::translate(mat4f{1.0F}, held_item_translation_);
    mat4f rotation{1.0F};
    rotation = glm::rotate(rotation, held_item_rotation_.x, vec3f{1.0F, 0.0F, 0.0F});
    rotation = glm::rotate(rotation, held_item_rotation_.y, vec3f{0.0F, 1.0F, 0.0F});
    rotation = glm::rotate(rotation, held_item_rotation_.z, vec3f{0.0F, 0.0F, 1.0F});
    const mat4f scale = glm::scale(mat4f{1.0F}, vec3f{held_item_scale_});
    return removeScale(attachment_global) * translation * rotation * scale;
}

bool ModelTestScene::fail(std::string message) {
    status_ = std::move(message);
    return false;
}
}  // namespace mle::user
