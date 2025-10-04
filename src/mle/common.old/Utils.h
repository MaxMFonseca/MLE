/**
 * @file Utils.h
 * @brief Core type definitions and utilities used across the engine.
 */

#pragma once

#include <algorithm>
#include <charconv>
#include <expected>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "Result.h"
#include "Types.h"
#include "mle/common/Logger.h"

namespace mle {
/**
 * @brief Splits a string by a given delimiter.
 *
 * @param s The input string to split.
 * @param delim The character delimiter to split the string on.
 * @return A vector containing the split substrings.
 */
[[maybe_unused]] [[nodiscard]] std::vector<std::string> split(const std::string& s, char delim);

/**
 * @brief Extracts a numeric value and its suffix from a string.
 *
 * @tparam T The numeric type to extract (e.g., int, float).
 * @param s The input string containing the number and suffix.
 * @return A pair consisting of the extracted number and the remaining suffix string.
 * @throws INVALID_ARGUMENT if the numeric extraction fails.
 */
template <typename T>
[[maybe_unused]] Expected<std::pair<T, std::string>> splitNumberAndSuffix(const std::string& s) {
    if (!std::isdigit(s[0]) && s[0] != '.') {
        return std::make_pair(0, s);
    }

    T result{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), result);  // NOLINT

    if (ec != std::errc()) {
        return std::unexpected(Result::INVALID_ARGUMENT);
    }

    return std::make_pair(result, std::string(ptr));
}

/// Converts a UTF-8 string to lowercase using ICU.
std::string toLower(const std::string& input);

/**
 * @brief Finds the N-th occurrence of a character in a string.
 *
 * @param str The string to search in.
 * @param ch The character to search for.
 * @param n Occurrence number to find.
 * @return std::optional<size_t> The index of the N-th occurrence if found;
 *         std::nullopt if the character appears fewer than N times.
 */
std::optional<size_t> findNthChar(const std::string& str, char ch, size_t n);

/**
 * @brief Checks if a character is a digit.
 *
 * @param c The character to check.
 * @return True if the character is a digit; otherwise, false.
 */
inline bool isDigit(char c) {
    return std::isdigit(c) != 0;
}

/**
 * @brief Logs the contents of a directory.
 *
 * @param path The path to the directory to log.
 */
void logFolder(const fs::path& path);

/**
 * @brief Reads the size of a file.
 *
 * @param path The path to the file.
 * @return The file size in bytes, or an error if the file cannot be read.
 */
Expected<usize> readFileSize(const fs::path& path);

/**
 * @brief Reads the contents of a file into a byte buffer.
 *
 * @param path The path to the file.
 * @param binary Whether to open the file in binary mode (default: true).
 * @return The contents of the file on success, or an error code on failure.
 */
Expected<Bytes> readFile(const fs::path& path, bool binary = true);

/**
 * @brief Reads the contents of a file as a string.
 *
 * @param path The path to the file.
 * @return The file contents as a string, or an error if the file cannot be read.
 */
Expected<std::string> readFileAsString(const fs::path& path);

/**
 * @brief Retrieves the entries in a directory.
 *
 * @param path The path to the directory.
 * @return A list of paths in the directory, or an error if the path is invalid or unreadable.
 */
Expected<std::vector<fs::path>> getEntriesInDirectory(const fs::path& path);

/**
 * @brief Recursively retrieves the file entries in a directory.
 *
 * @param path The root directory to scan.
 * @return A list of regular file paths found recursively, or an error code on failure.
 */
Expected<std::vector<fs::path>> getEntriesInDirectoryRecursive(const fs::path& path);

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
Expected<std::vector<fs::path>> evaluateFilePathPatterns(const std::vector<fs::path>& paths);

/**
 * @brief Processes a list of file path strings, supporting inclusion and exclusion patterns.
 *
 * This function interprets each string in the input vector. Strings starting with '-' are treated as exclusions.
 * If such a string ends with '*', it recursively excludes all entries in the specified directory.
 * Strings ending with '*' are treated as inclusions, recursively including all entries in the specified directory.
 * The final result is the set difference between included and excluded paths.
 *
 * @param paths Vector of file path strings to process.
 * @return Vector of resulting file paths after applying inclusion and exclusion rules, or an error.
 */
Expected<std::vector<fs::path>> evaluateFilePathPatterns(const std::vector<std::string>& paths);

/**
 * @brief Adds a prefix to a string, optionally after a specified substring.
 *
 * If the 'after' parameter is empty, the prefix is added at the beginning of the string.
 * If the string starts with the 'after' substring, the prefix is inserted immediately after it.
 *
 * @param[in,out] s The string to modify.
 * @param[in] prefix The prefix to add.
 * @param[in] after Optional substring to insert the prefix after.
 */
void insertPrefix(std::string& s, const std::string& prefix, const std::string& after = "");

/**
 * @brief Adds a prefix to each string in a vector, optionally after a specified substring.
 *
 * Applies the same logic as the single string version to each element in the vector.
 *
 * @param[in,out] v The vector of strings to modify.
 * @param[in] prefix The prefix to add.
 * @param[in] after Optional substring to insert the prefix after.
 */
void insertPrefix(std::vector<std::string>& v, const std::string& prefix, const std::string& after = "");

/**
 * @brief Generates a unique identifier.
 *
 * @return A unique ID.
 */
ID genID();

namespace idvec {
/**
 * @brief Retrieves an element from an ID vector by its ID.
 *
 * @tparam T The type of the elements in the ID vector.
 * @param v The ID vector to search.
 * @param id The ID of the element to retrieve.
 * @return A reference to the element if found, or an error.
 */
template <typename T>
Expected<std::reference_wrapper<T>> get(IDVec<T>& v, ID id) {
    auto it = std::find_if(v.begin(), v.end(), [id](const auto& p) { return p.first == id; });
    if (it == v.end()) {
        return std::unexpected(Result::NOT_FOUND);
    }
    return std::ref(it->second);
}

/**
 * @brief Retrieves an element from an ID vector by its ID (const version).
 *
 * @tparam T The type of the elements in the ID vector.
 * @param v The constant ID vector to search.
 * @param id The ID of the element to retrieve.
 * @return A const reference to the element if found, or an error.
 */
template <typename T>
Expected<std::reference_wrapper<const T>> get(const IDVec<T>& v, ID id) {
    auto it = std::find_if(v.begin(), v.end(), [id](const auto& p) { return p.first == id; });
    if (it == v.end()) {
        return std::unexpected(Result::NOT_FOUND);
    }
    return std::cref(it->second);
}

/**
 * @brief Removes an element from an ID vector by its ID.
 *
 * @tparam T The type of the elements in the ID vector.
 * @param v The ID vector to modify.
 * @param id The ID of the element to remove.
 * @return Success or an error if the ID was not found.
 */
template <typename T>
Expected<void> remove(IDVec<T>& v, ID id) {
    auto it = std::find_if(v.begin(), v.end(), [id](const auto& p) { return p.first == id; });
    if (it == v.end()) {
        return std::unexpected(Result::NOT_FOUND);
    }
    v.erase(it);
    return {};
}

/**
 * @brief Adds a new element to an ID vector and assigns it a unique ID.
 *
 * @tparam T The type of the element to add.
 * @param v The ID vector to modify.
 * @param t The element to add.
 * @return The unique ID assigned to the new element.
 */
template <typename T>
ID emplaceBack(IDVec<T>& v, T&& t) {
    auto id = genID();
    v.emplace_back(id, std::forward<T>(t));
    return id;
}

/**
 * @brief Adds a new element to an ID vector with a specified ID.
 *
 * @tparam T The type of the element to add.
 * @param v The ID vector to modify.
 * @param id The ID to assign to the new element.
 * @param t The element to add.
 */
template <typename T>
void emplaceBack(IDVec<T>& v, ID id, T&& t) {
    v.emplace_back(id, std::forward<T>(t));
}

}  // namespace idvec

/**
 * @brief Reads the contents of a file into a buffer.
 *
 * @tparam T A buffer type supporting `.data()` and `.size()`.
 * @param path The path to the file.
 * @param buffer The buffer to read the file into.
 * @param size The number of bytes to read (default: 0, which uses buffer size).
 * @param binary Whether to open the file in binary mode (default: true).
 * @return Result::OK on success, or an appropriate error code on failure.
 */
template <typename T>
Result readFile(const fs::path& path, T& buffer, usize size = 0, bool binary = true) {
    MLE_T("Reading file: {}", path.generic_string());

    std::ifstream file;
    file.exceptions(std::ios::goodbit);  // Disable exceptions

    file.open(path, binary ? std::ios::binary : std::ios::in);
    if (!file.is_open()) {
        MLE_E("Failed to open file: {}", path.generic_string());
        return Result::FAILED_TO_OPEN;
    }

    const usize to_read = size ? size : buffer.size();
    file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(to_read));
    if (!file) {
        MLE_E("Failed to read file: {}", path.generic_string());
        return Result::FAILED_TO_OPEN;
    }

    return Result::OK;
}
}  // namespace mle
