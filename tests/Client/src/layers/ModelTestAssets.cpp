#include "ModelTestAssets.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <ranges>
#include <unordered_set>

#include "mle/renderer/GLTF.h"

namespace mle::user {
namespace {
std::string lowerCopy(std::string value) {
    std::ranges::transform(value, value.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

bool pathStartsWith(const Path& path, const Path& base) {
    auto path_it = path.begin();
    for (auto base_it = base.begin(); base_it != base.end(); ++base_it, ++path_it) {
        if (path_it == path.end() || *path_it != *base_it) {
            return false;
        }
    }
    return true;
}

std::expected<Path, std::string> canonicalPath(const Path& path, std::string_view label) {
    std::error_code ec;
    Path canonical = std::filesystem::weakly_canonical(path, ec);
    if (ec) {
        return std::unexpected{std::string{"Could not resolve "} + std::string{label} + ": " + ec.message()};
    }
    return canonical;
}

std::string longestCommonPrefix(const std::vector<std::string>& values) {
    if (values.empty()) {
        return {};
    }

    std::string prefix = values.front();
    for (auto it = std::next(values.begin()); it != values.end(); ++it) {
        const usize common = std::ranges::mismatch(prefix, *it).in1 - prefix.begin();
        prefix.resize(common);
    }
    return prefix;
}

void sortAndUnique(std::vector<std::string>& values) {
    std::ranges::sort(values);
    values.erase(std::unique(values.begin(), values.end()), values.end());
}

void collectSceneMeshNodes(const tinygltf::Model& model, int node_index, std::vector<std::pair<std::string, usize>>& out, std::unordered_set<usize>& visiting) {
    if (node_index < 0 || node_index >= static_cast<int>(model.nodes.size())) {
        return;
    }

    const usize index = static_cast<usize>(node_index);
    if (!visiting.insert(index).second) {
        return;
    }

    const auto& node = model.nodes[index];
    if (node.mesh >= 0) {
        const std::string name = node.name.empty() ? "node_" + std::to_string(index) : node.name;
        out.emplace_back(name, index);
    }
    for (int child : node.children) {
        collectSceneMeshNodes(model, child, out, visiting);
    }
    visiting.erase(index);
}

std::expected<AssetMetadata, std::string> loadMetadata(const Path& path) {
    GLTF gltf;
    if (gltf.load(path) != Result::OK) {
        return std::unexpected{"Failed to load GLTF metadata for " + path.generic_string()};
    }

    const auto& model = gltf.model();
    AssetMetadata metadata;
    metadata.node_names.reserve(model.nodes.size());
    for (const auto& node : model.nodes) {
        if (!node.name.empty()) {
            metadata.node_names.push_back(node.name);
        }
    }

    metadata.animation_names.reserve(model.animations.size());
    for (const auto& animation : model.animations) {
        if (!animation.name.empty()) {
            metadata.animation_names.push_back(animation.name);
        }
    }

    const int scene_index = gltf.defaultSceneIndex();
    if (scene_index >= 0 && scene_index < static_cast<int>(model.scenes.size())) {
        std::unordered_set<usize> visiting;
        for (int node_index : model.scenes[static_cast<usize>(scene_index)].nodes) {
            collectSceneMeshNodes(model, node_index, metadata.mesh_nodes, visiting);
        }
    }
    return metadata;
}
}  // namespace

std::string ModelResourceId::normalized() const {
    if (selector.has_value()) {
        return path + "#" + *selector;
    }
    return path;
}

ModelTestAssets::ModelTestAssets(Path models_root, MetadataLoader loader) :
    models_root_(std::move(models_root)),
    loader_(std::move(loader)) {
    if (!loader_) {
        loader_ = loadMetadata;
    }
}

std::expected<ModelResourceId, std::string> ModelTestAssets::parseResourceId(std::string_view text) const {
    if (text.empty()) {
        return std::unexpected{"Resource path is empty"};
    }

    const usize separator = text.find('#');
    if (separator != std::string_view::npos && text.find('#', separator + 1) != std::string_view::npos) {
        return std::unexpected{"Resource ID may contain only one # selector separator"};
    }

    const std::string_view path_text = text.substr(0, separator);
    std::optional<std::string> selector;
    if (separator != std::string_view::npos) {
        const std::string_view selector_text = text.substr(separator + 1);
        if (selector_text.empty()) {
            return std::unexpected{"Resource selector after # is empty"};
        }
        selector = std::string{selector_text};
    }

    const Path input_path{path_text};
    if (input_path.empty()) {
        return std::unexpected{"Resource path is empty"};
    }
    if (input_path.is_absolute() || input_path.has_root_path()) {
        return std::unexpected{"Resource path must be relative to the models root"};
    }
    if (std::ranges::any_of(input_path, [](const auto& component) { return component == ".."; })) {
        return std::unexpected{"Resource path may not escape the models root with .."};
    }

    const Path normalized_path = input_path.lexically_normal();
    auto component = normalized_path.begin();
    if (component == normalized_path.end() || *component != "i") {
        return std::unexpected{"Resource path must begin with i/"};
    }
    ++component;
    if (component == normalized_path.end()) {
        return std::unexpected{"Resource path must include a file below i/"};
    }

    const std::string extension = lowerCopy(normalized_path.extension().generic_string());
    if (extension != ".glb" && extension != ".gltf") {
        return std::unexpected{"Resource path must end in .glb or .gltf"};
    }

    const auto canonical_user_root = canonicalPath(models_root_ / "i", "user models root");
    if (!canonical_user_root.has_value()) {
        return std::unexpected{canonical_user_root.error()};
    }
    const auto canonical_candidate = canonicalPath(models_root_ / normalized_path, "resource path");
    if (!canonical_candidate.has_value()) {
        return std::unexpected{canonical_candidate.error()};
    }
    if (!pathStartsWith(*canonical_candidate, *canonical_user_root)) {
        return std::unexpected{"Resource path escapes the user models root"};
    }

    return ModelResourceId{
        .path = normalized_path.generic_string(),
        .selector = std::move(selector),
    };
}

std::expected<std::vector<std::string>, std::string> ModelTestAssets::scanUserPaths() {
    user_paths_.clear();
    resetCompletion();

    const Path base = models_root_ / "i";
    std::error_code ec;
    const bool exists = std::filesystem::exists(base, ec);
    if (ec) {
        return std::unexpected{"Could not inspect user model directory: " + ec.message()};
    }
    if (!exists) {
        return user_paths_;
    }
    const bool is_directory = std::filesystem::is_directory(base, ec);
    if (ec) {
        return std::unexpected{"Could not inspect user model directory: " + ec.message()};
    }
    if (!is_directory) {
        return std::unexpected{"User model path is not a directory: " + base.generic_string()};
    }

    std::filesystem::recursive_directory_iterator it{base, std::filesystem::directory_options::none, ec};
    const std::filesystem::recursive_directory_iterator end;
    if (ec) {
        return std::unexpected{"Could not scan user model directory: " + ec.message()};
    }

    while (it != end) {
        const auto entry = *it;
        it.increment(ec);
        if (ec) {
            return std::unexpected{"Could not scan user model directory: " + ec.message()};
        }

        std::error_code file_ec;
        const bool is_regular_file = entry.is_regular_file(file_ec);
        if (file_ec) {
            return std::unexpected{"Could not inspect user model entry '" + entry.path().generic_string() + "': " + file_ec.message()};
        }
        if (!is_regular_file) {
            continue;
        }
        const Path relative = entry.path().lexically_relative(models_root_);
        const auto parsed = parseResourceId(relative.generic_string());
        if (parsed.has_value()) {
            user_paths_.push_back(parsed->path);
        }
    }

    sortAndUnique(user_paths_);
    return user_paths_;
}

std::expected<std::reference_wrapper<const AssetMetadata>, std::string> ModelTestAssets::metadataFor(std::string_view path) {
    const auto parsed = parseResourceId(path);
    if (!parsed.has_value()) {
        return std::unexpected{parsed.error()};
    }
    if (parsed->selector.has_value()) {
        return std::unexpected{"Metadata path must not include a # selector"};
    }

    if (const auto cached = metadata_cache_.find(parsed->path); cached != metadata_cache_.end()) {
        if (!cached->second.has_value()) {
            return std::unexpected{cached->second.error()};
        }
        return std::cref(cached->second.value());
    }

    const auto canonical_root = canonicalPath(models_root_, "models root");
    if (!canonical_root.has_value()) {
        return std::unexpected{canonical_root.error()};
    }
    const auto absolute_path = canonicalPath(*canonical_root / parsed->path, "resource path");
    if (!absolute_path.has_value()) {
        return std::unexpected{absolute_path.error()};
    }

    std::error_code ec;
    const bool is_regular_file = std::filesystem::is_regular_file(*absolute_path, ec);
    if (ec) {
        return std::unexpected{"Could not inspect resource path '" + parsed->path + "': " + ec.message()};
    }
    if (!is_regular_file) {
        return std::unexpected{"Resource path is not a regular file: " + parsed->path};
    }

    auto [cached, inserted] = metadata_cache_.emplace(parsed->path, loader_(*absolute_path));
    (void)inserted;
    if (!cached->second.has_value()) {
        return std::unexpected{cached->second.error()};
    }
    return std::cref(cached->second.value());
}

CompletionResult ModelTestAssets::complete(ModelResourceKind kind, std::string_view query, std::span<const std::string> attachment_nodes) {
    std::vector<std::string> candidates;
    if (kind == ModelResourceKind::ATTACHMENT_NODE) {
        std::vector<std::string> context;
        context.reserve(attachment_nodes.size());
        for (const auto& node : attachment_nodes) {
            context.push_back(node);
            if (node.starts_with(query)) {
                candidates.push_back(node);
            }
        }
        return finishCompletion(kind, query, std::move(candidates), std::move(context), true);
    }

    const usize separator = query.find('#');
    if (separator == std::string_view::npos) {
        for (const auto& path : user_paths_) {
            if (path.starts_with(query)) {
                candidates.push_back(path);
            }
        }
        return finishCompletion(kind, query, std::move(candidates), user_paths_);
    }
    if (query.find('#', separator + 1) != std::string_view::npos) {
        resetCompletion();
        return CompletionResult{.replacement = std::string{query}, .suggestions = {}, .message = "Resource ID may contain only one # selector separator"};
    }

    const std::string path{query.substr(0, separator)};
    if (std::ranges::find(user_paths_, path) == user_paths_.end()) {
        resetCompletion();
        return CompletionResult{.replacement = std::string{query}, .suggestions = {}, .message = "Complete an exact resource path before its selector"};
    }

    const auto metadata = metadataFor(path);
    if (!metadata.has_value()) {
        resetCompletion();
        return CompletionResult{
            .replacement = std::string{query},
            .suggestions = {},
            .message = metadata.error(),
        };
    }

    const std::string_view selector_prefix = query.substr(separator + 1);
    std::vector<std::string> context;
    if (kind == ModelResourceKind::ANIMATION) {
        for (const auto& name : metadata->get().animation_names) {
            context.push_back(path + "#" + name);
            if (name.starts_with(selector_prefix)) {
                candidates.push_back(path + "#" + name);
            }
        }
    } else {
        for (const auto& [name, index] : metadata->get().mesh_nodes) {
            (void)index;
            context.push_back(path + "#" + name);
            if (name.starts_with(selector_prefix)) {
                candidates.push_back(path + "#" + name);
            }
        }
    }
    return finishCompletion(kind, query, std::move(candidates), std::move(context), true);
}

CompletionResult ModelTestAssets::finishCompletion(ModelResourceKind kind, std::string_view query, std::vector<std::string> candidates,
                                                   std::vector<std::string> context, bool preserve_duplicates) {
    std::ranges::sort(candidates);
    std::ranges::sort(context);
    if (!preserve_duplicates) {
        candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
        context.erase(std::unique(context.begin(), context.end()), context.end());
    }

    if (cycle_kind_ == kind && cycle_context_ == context && !cycle_candidates_.empty() && (query == cycle_initial_query_ || query == cycle_last_output_)) {
        if (!cycle_started_) {
            cycle_index_ = 0;
            cycle_started_ = true;
        } else {
            cycle_index_ = (cycle_index_ + 1) % cycle_candidates_.size();
        }
        cycle_last_output_ = cycle_candidates_[cycle_index_];
        return CompletionResult{
            .replacement = cycle_last_output_,
            .suggestions = cycle_candidates_,
            .message = std::to_string(cycle_index_ + 1) + " of " + std::to_string(cycle_candidates_.size()) + " completions",
        };
    }

    if (candidates.empty()) {
        resetCompletion();
        return CompletionResult{
            .replacement = std::string{query},
            .suggestions = {},
            .message = "No completion",
        };
    }
    if (candidates.size() == 1) {
        resetCompletion();
        return CompletionResult{
            .replacement = candidates.front(),
            .suggestions = std::move(candidates),
            .message = {},
        };
    }

    std::string replacement = longestCommonPrefix(candidates);
    cycle_kind_ = kind;
    cycle_candidates_ = candidates;
    cycle_context_ = std::move(context);
    cycle_initial_query_ = query;
    cycle_last_output_ = replacement;
    cycle_index_ = 0;
    const auto replacement_it = std::ranges::find(cycle_candidates_, replacement);
    cycle_started_ = replacement_it != cycle_candidates_.end();
    if (cycle_started_) {
        cycle_index_ = static_cast<usize>(replacement_it - cycle_candidates_.begin());
    } else if (replacement == query) {
        replacement = cycle_candidates_.front();
        cycle_last_output_ = replacement;
        cycle_started_ = true;
    }
    return CompletionResult{
        .replacement = replacement,
        .suggestions = std::move(candidates),
        .message = std::to_string(cycle_candidates_.size()) + " completions",
    };
}

void ModelTestAssets::resetCompletion() {
    cycle_kind_.reset();
    cycle_candidates_.clear();
    cycle_context_.clear();
    cycle_initial_query_.clear();
    cycle_last_output_.clear();
    cycle_index_ = 0;
    cycle_started_ = false;
}
}  // namespace mle::user
