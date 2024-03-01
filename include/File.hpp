#pragma once

#include <vector>
#include <string>
#include <string_view>

namespace SongCore::Utils {
    std::vector<std::string> GetFolders(std::string_view path);
    std::u16string ReadText(std::string_view path);
    const char* ReadBytes(std::string_view path, size_t& size_out);
}