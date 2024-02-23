#pragma once

#include <vector>
#include <string_view>

namespace SongCore::Utils {
    std::vector<std::string> GetFolders(std::string_view path);
}