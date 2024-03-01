#include "File.hpp"
#include "logging.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include <filesystem>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>
#include <filesystem>
#include <fstream>

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

std::u16string SongCore::Utils::ReadText(std::string_view path){
    if(!fileexists(path))
        return u"";
    std::basic_ifstream<char16_t> fileStream(path.data(), std::ifstream::in);
    if(!fileStream.is_open())
        return u"";
    std::u16string text((std::istreambuf_iterator<char16_t>(fileStream)), std::istreambuf_iterator<char16_t>());
    return text;
}

const char *SongCore::Utils::ReadBytes(std::string_view path, size_t& size_out) {
    size_out = 0;
    if(!fileexists(path))
        return nullptr;
    std::ifstream fileStream(path.data(), std::ifstream::in | std::ios::binary | std::ifstream::ate);
    size_out = fileStream.tellg();
    if(!fileStream.is_open())
        return nullptr;
    char* data = new char[size_out];
    fileStream.seekg(fileStream.beg);
    fileStream.read(data, size_out);
    return data;
}
