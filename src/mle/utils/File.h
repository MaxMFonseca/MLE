#pragma once

#include <expected>
#include <filesystem>
#include <fstream>

#include "mle/core/Logger.h"
#include "mle/core/Result.h"
#include "mle/utils/Types.h"

namespace mle {
using Path = std::filesystem::path;

Expected<Bytes> readFileBytes(const Path& path);
Expected<std::string> readFileString(const Path& path);

Expected<std::vector<Path>> getEntriesInDirectory(const Path& path, bool recursive = false);

}  // namespace mle
