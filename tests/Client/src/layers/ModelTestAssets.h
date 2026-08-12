#pragma once

#include <expected>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "mle/utils/File.h"

namespace mle::user {
enum class ModelResourceKind {
    MODEL,
    HELD_ITEM,
    ANIMATION,
    ATTACHMENT_NODE,
};

struct ModelResourceId {
    std::string path;
    std::optional<std::string> selector;

    [[nodiscard]] std::string normalized() const;
};

struct AssetMetadata {
    std::vector<std::pair<std::string, usize>> mesh_nodes;
    std::vector<std::string> node_names;
    std::vector<std::string> animation_names;
};

struct CompletionResult {
    std::string replacement;
    std::vector<std::string> suggestions;
    std::string message;
};

using MetadataLoader = std::function<std::expected<AssetMetadata, std::string>(const Path&)>;

class ModelTestAssets {
  public:
    explicit ModelTestAssets(Path models_root, MetadataLoader loader = {});
    std::expected<std::vector<std::string>, std::string> scanUserPaths();
    std::expected<ModelResourceId, std::string> parseResourceId(std::string_view text) const;
    std::expected<std::reference_wrapper<const AssetMetadata>, std::string> metadataFor(std::string_view path);
    CompletionResult complete(ModelResourceKind kind, std::string_view query, std::span<const std::string> attachment_nodes = {});

  private:
    CompletionResult finishCompletion(ModelResourceKind kind, std::string_view query, std::vector<std::string> candidates, std::vector<std::string> context,
                                      bool preserve_duplicates = false);
    void resetCompletion();

    Path models_root_;
    MetadataLoader loader_;
    std::vector<std::string> user_paths_;
    std::unordered_map<std::string, std::expected<AssetMetadata, std::string>> metadata_cache_;

    std::optional<ModelResourceKind> cycle_kind_;
    std::vector<std::string> cycle_candidates_;
    std::vector<std::string> cycle_context_;
    std::string cycle_initial_query_;
    std::string cycle_last_output_;
    usize cycle_index_ = 0;
    bool cycle_started_ = false;
};
}  // namespace mle::user
