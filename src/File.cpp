#include "File.hpp"

#include <filesystem>
#include <string_view>
#include <system_error>
#include <vector>
#include <filesystem>

std::vector<std::string> SongCore::Utils::GetFolders(std::string_view path) {
    std::vector<std::string> dirs;
    if (!std::filesystem::is_directory(path)) {
        return dirs;
    }

    std::error_code error;
    auto directory_iter = std::filesystem::directory_iterator(path, std::filesystem::directory_options::none, error);

    for (auto const& entry : directory_iter) {
        if (entry.is_directory()) {
            dirs.push_back(entry.path().string());
        }
    }

    return dirs;
}