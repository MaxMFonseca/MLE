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

/// Disable copy constructor and assignment operator for a class.
#define MLE_NO_COPY(Class)        \
    Class(const Class&) = delete; \
    Class& operator=(const Class&) = delete;

/// Disable move constructor and assignment operator for a class.
#define MLE_NO_MOVE(Class)   \
    Class(Class&&) = delete; \
    Class& operator=(Class&&) = delete;

/// Disable both copy and move constructors and assignment operators for a class.
#define MLE_NO_COPY_MOVE(Class) \
    MLE_NO_COPY(Class)          \
    MLE_NO_MOVE(Class)

/// No operation macro
#define MLE_NOOP ((void)0)

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

/**
 * @brief Performs a `static_cast` from one type to another.
 *
 * This utility provides a clearer and more concise syntax for static casting,
 * similar to the `as` keyword in other languages (e.g., Rust, C#).
 * I wish C++ had a built-in `as` keyword for this purpose.
 * Imagine... (a as f32) instead of static_cast<f32>(a).
 *
 * @tparam T The target type to cast to.
 * @tparam U The source type.
 * @param value The value to cast.
 * @return The value statically casted to type `T`.
 */
template <typename T, typename U>
constexpr T as(U&& value) {
    return static_cast<T>(std::forward<U>(value));
}

/// Alias for reinterpret_cast.
template <typename T, typename U>
constexpr T rAs(U value) {
    return reinterpret_cast<T>(std::forward<U>(value));
}

/**
 * @brief Converts a raw void pointer to a reference of type `T&`.
 *
 * @tparam T The target reference type.
 * @param value A `void*` pointing to an object of type `T`.
 * @return A reference to the reinterpreted object.
 */
template <typename T>
constexpr T& rVoidAsRef(void* value) {
    return *rAs<T*>(value);
}

namespace res {
/// Joins segments with '/'.
inline std::string joinPath(std::string_view a, std::string_view b) {
    return std::string(a) + "/" + std::string(b);
}

/// Adds a resource base path to a file.
inline std::string addBasePath(std::string_view file, std::string_view domain, std::string_view typeDir) {
    return joinPath(joinPath(joinPath(RES_BASE_PATH, domain), typeDir), file);
}

/// Removes the first 3 segments: "res/<domain>/<type>/" from a full path.
inline std::string removeBasePath(std::string_view fullPath) {
    if (auto pos = findNthChar(std::string(fullPath), '/', 3)) {
        return std::string(fullPath.substr(*pos + 1));
    }
    return std::string(fullPath);  // Return unchanged if format doesn't match
}

/// Adds the MLE font path to a file.
inline std::string addMleFontPath(std::string_view file) {
    return addBasePath(file, MLE_RES_SUBDIR_NAME, ResPath::FONTS);
}

/// Adds the MLE shader path to a file.
inline std::string addMleShaderPath(std::string_view file) {
    return addBasePath(file, MLE_RES_SUBDIR_NAME, ResPath::SHADERS);
}

/// Adds the MLE Lua script path to a file.
inline std::string addMleLuaPath(std::string_view file) {
    return addBasePath(file, MLE_RES_SUBDIR_NAME, ResPath::LUA);
}

/// Adds the MLE texture path to a file.
inline std::string addMleTexturePath(std::string_view file) {
    return addBasePath(file, MLE_RES_SUBDIR_NAME, ResPath::TEXTURES);
}

/// Adds the user font path to a file.
inline std::string addUserFontPath(std::string_view file) {
    return addBasePath(file, USER_RES_SUBDIR_NAME, ResPath::FONTS);
}

/// Adds the user shader path to a file.
inline std::string addUserShaderPath(std::string_view file) {
    return addBasePath(file, USER_RES_SUBDIR_NAME, ResPath::SHADERS);
}

/// Adds the user Lua script path to a file.
inline std::string addUserLuaPath(std::string_view file) {
    return addBasePath(file, USER_RES_SUBDIR_NAME, ResPath::LUA);
}

/// Adds the user texture path to a file.
inline std::string addUserTexturePath(std::string_view file) {
    return addBasePath(file, USER_RES_SUBDIR_NAME, ResPath::TEXTURES);
}
}  // namespace res
}  // namespace mle
