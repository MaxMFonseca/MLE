#include "File.h"

namespace mle {
Expected<Bytes> readFileBytes(const Path& path) {
    Bytes ret;

    std::ifstream file;
    file.exceptions(std::ios::goodbit);  // Disable exceptions

    file.open(path, std::ios::binary);
    if (!file.is_open()) {
        MLE_E("Failed to open file: {}", path.generic_string());
        return std::unexpected(Result::FAILED_TO_OPEN);
    }

    usize size = 0;
    file.seekg(0, std::ios::end);
    size = as<usize>(file.tellg());
    file.seekg(0, std::ios::beg);
    ret.resize(size);

    file.read(rAs<char*>(ret.data()), as<std::streamsize>(size));
    if (!file) {
        MLE_E("Failed to read file: {}", path.generic_string());
        return std::unexpected(Result::FAILED_TO_OPEN);
    }

    return ret;
}

Expected<std::string> readFileString(const Path& path) {
    std::string ret;

    std::ifstream file;
    file.exceptions(std::ios::goodbit);  // Disable exceptions

    file.open(path, std::ios::in);
    if (!file.is_open()) {
        MLE_E("Failed to open file: {}", path.generic_string());
        return std::unexpected(Result::FAILED_TO_OPEN);
    }

    usize size = 0;
    file.seekg(0, std::ios::end);
    size = as<usize>(file.tellg());
    file.seekg(0, std::ios::beg);
    ret.resize(size);

    file.read(ret.data(), as<std::streamsize>(size));
    if (!file) {
        MLE_E("Failed to read file: {}", path.generic_string());
        return std::unexpected(Result::FAILED_TO_OPEN);
    }

    return ret;
}

Expected<std::vector<Path>> getEntriesInDirectory(const Path& path, bool recursive) {
    std::vector<Path> entries;
    std::error_code ec;

    if (!std::filesystem::exists(path, ec) || !std::filesystem::is_directory(path, ec)) {
        MLE_E("Path is not a directory: {}", path.generic_string());
        return std::unexpected(Result::FAILED_TO_OPEN);
    }

    if (recursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path, ec)) {
            if (ec) {
                MLE_E("Error while iterating directory: {}", ec.message());
                return std::unexpected(Result::FAILED_TO_OPEN);
            }
            entries.push_back(entry.path());
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
            if (ec) {
                MLE_E("Error while iterating directory: {}", ec.message());
                return std::unexpected(Result::FAILED_TO_OPEN);
            }
            entries.push_back(entry.path());
        }
    }
    return entries;
}
}  // namespace mle
