#pragma once

#include "CustomJSONData.hpp"

namespace SongCore::Utils {
    std::optional<std::string> GetCustomLevelHash(std::filesystem::path levelPath, SongCore::CustomJSONData::CustomLevelInfoSaveData* saveData);
}
