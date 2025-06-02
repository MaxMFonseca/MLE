#include "Utils.h"

#include <unicode/unistr.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

#include "mle/common/Consts.h"
#include "mle/common/Logger.h"

namespace mle {
ID genID() {
    static ID id = 0;
    return id++;
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, delim)) {
        result.push_back(item);
    }

    return result;
}

std::string toLower(const std::string& input) {
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(input);
    ustr.toLower();
    std::string result;
    ustr.toUTF8String(result);
    return result;
}

std::optional<size_t> findNthChar(const std::string& str, char ch, size_t n) {
    size_t count = 0;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == ch) {
            ++count;
            if (count == n) {
                return i;
            }
        }
    }
    return std::nullopt;
}

void logFolder(const fs::path& path) {
    MLE_D("Folder: {}", path.generic_string());
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        MLE_D("  {}", entry.path().generic_string());
    }
}

Expected<usize> readFileSize(const fs::path& path) {
    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    if (ec) {
        MLE_E("Failed to stat file '{}': {}", path.generic_string(), ec.message());
        return std::unexpected(Result::FAILED_TO_OPEN_FILE);
    }
    return static_cast<usize>(size);
}

Expected<Bytes> readFile(const fs::path& path, bool binary) {
    MLE_T("Reading file: {}", path.generic_string());

    std::ifstream file;
    file.exceptions(std::ios::goodbit);

    file.open(path, binary ? std::ios::ate | std::ios::binary : std::ios::ate);
    if (!file.is_open()) {
        MLE_E("Failed to open file: {}", path.generic_string());
        return std::unexpected(Result::FAILED_TO_OPEN_FILE);
    }

    std::streamsize file_size = file.tellg();
    if (file_size <= 0) {
        MLE_E("File is empty or unreadable: {}", path.generic_string());
        return std::unexpected(Result::FAILED_TO_OPEN_FILE);
    }

    Bytes buffer(static_cast<usize>(file_size));

    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), file_size);
    if (!file) {
        MLE_E("Failed to read file: {}", path.generic_string());
        return std::unexpected(Result::FAILED_TO_OPEN_FILE);
    }

    return buffer;
}

Expected<std::string> readFileAsString(const fs::path& path) {
    MLE_T("Reading file: {}", path.generic_string());

    std::ifstream file;
    file.exceptions(std::ios::goodbit);  // Disable throwing

    file.open(path, std::ios::ate);
    if (!file.is_open()) {
        MLE_E("Failed to open file: {}", path.generic_string());
        return std::unexpected(Result::FAILED_TO_OPEN_FILE);
    }

    std::streamsize file_size = file.tellg();
    if (file_size <= 0) {
        MLE_E("File is empty or unreadable: {}", path.generic_string());
        return std::unexpected(Result::FAILED_TO_OPEN_FILE);
    }

    std::string ret(static_cast<usize>(file_size), '\0');

    file.seekg(0);
    file.read(ret.data(), file_size);
    if (!file) {
        MLE_E("Failed to read file: {}", path.generic_string());
        return std::unexpected(Result::FAILED_TO_OPEN_FILE);
    }

    return ret;
}

Expected<std::vector<fs::path>> getEntriesInDirectory(const fs::path& path) {
    std::vector<fs::path> entries;

    std::error_code ec;
    if (!fs::exists(path, ec) || !fs::is_directory(path, ec)) {
        MLE_E("Path is not a directory: {}", path.generic_string());
        return std::unexpected(Result::INVALID_ARGUMENT);
    }

    for (fs::directory_iterator it(path, ec), end; it != end; it.increment(ec)) {
        if (ec) {
            MLE_E("Error iterating directory {}: {}", path.generic_string(), ec.message());
            return std::unexpected(Result::FAILED_TO_OPEN_FILE);
        }
        entries.push_back(it->path());
    }

    return entries;
}

Expected<std::vector<fs::path>> getEntriesInDirectoryRecursive(const fs::path& path) {
    std::vector<fs::path> entries;

    std::error_code ec;
    if (!fs::exists(path, ec) || !fs::is_directory(path, ec)) {
        MLE_E("Path is not a directory: {}", path.generic_string());
        return std::unexpected(Result::INVALID_ARGUMENT);
    }

    for (fs::recursive_directory_iterator it(path, ec), end; it != end; it.increment(ec)) {
        if (ec) {
            MLE_E("Error during recursive directory iteration on {}: {}", path.generic_string(), ec.message());
            return std::unexpected(Result::FAILED_TO_OPEN_FILE);
        }

        if (it->is_regular_file(ec)) {
            if (ec) {
                MLE_E("Error checking file type: {}", ec.message());
                return std::unexpected(Result::FAILED_TO_OPEN_FILE);
            }
            entries.push_back(it->path());
        }
    }

    return entries;
}

/**
 * @brief Processes a list of file paths, supporting inclusion and exclusion patterns.
 *
 * This function interprets each path in the input vector. Paths starting with '-' are treated as exclusions.
 * If such a path ends with '*', it recursively excludes all entries in the specified directory.
 * Paths ending with '*' are treated as inclusions, recursively including all entries in the specified directory.
 * The final result is the set difference between included and excluded paths.
 *
 * @param paths Vector of file paths to process.
 * @return Vector of resulting file paths after applying inclusion and exclusion rules, or an error.
 */
Expected<std::vector<fs::path>> evaluateFilePathPatterns(const std::vector<fs::path>& paths) {
    std::vector<fs::path> all;
    std::vector<fs::path> remove;

    for (const auto& p : paths) {
        auto s = p.generic_string();

        if (s.starts_with('-')) {
            if (s.ends_with('*')) {
                auto result = getEntriesInDirectoryRecursive(fs::path(s.substr(1, s.size() - 2)));
                if (!result) {
                    return std::unexpected(result.error());
                }
                remove.insert(remove.end(), result->begin(), result->end());
            } else {
                remove.emplace_back(s.substr(1));
            }
            continue;
        }

        if (s.ends_with('*')) {
            auto result = getEntriesInDirectoryRecursive(fs::path(s.substr(0, s.size() - 1)));
            if (!result) {
                return std::unexpected(result.error());
            }
            all.insert(all.end(), result->begin(), result->end());
        } else {
            all.emplace_back(s);
        }
    }

    std::vector<fs::path> ret;
    std::ranges::set_difference(all, remove, std::back_inserter(ret));
    return ret;
}

void insertPrefix(std::string& s, const std::string& prefix, const std::string& after) {
    if (after.empty()) {
        s = prefix + s;
        return;
    }
    if (s.starts_with(after)) {
        s = after + prefix + s.substr(after.size());
    } else {
        s = prefix + s;
    }
}

void insertPrefix(std::vector<std::string>& v, const std::string& prefix, const std::string& after) {
    for (auto& s : v) {
        insertPrefix(s, prefix, after);
    }
}
}  // namespace mle
