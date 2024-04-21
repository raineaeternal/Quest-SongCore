#pragma once

#include "CustomJSONData.hpp"

namespace SongCore::Utils {
    std::optional<std::string> GetCustomLevelHash(std::filesystem::path const& levelPath, SongCore::CustomJSONData::CustomLevelInfoSaveData* saveData);
    std::optional<std::string> GetCustomLevelHash(std::filesystem::path const& levelPath, SongCore::CustomJSONData::CustomBeatmapLevelSaveData* saveData);
    std::optional<int> GetDirectoryHash(std::filesystem::path const& directoryPath);
}
