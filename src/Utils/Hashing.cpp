#include "Utils/Hashing.hpp"
#include "logging.hpp"
#include <filesystem>

namespace SongCore::Utils {
    std::optional<std::string> GetCustomLevelHash(std::filesystem::path levelPath, SongCore::CustomJSONData::CustomLevelInfoSaveData* saveData) {
        // TODO: implement hashing properly
        return fmt::format("{:x}_not_implemented", std::hash<std::string>()(levelPath));
    }

    std::optional<int> GetDirectoryHash(std::filesystem::path directoryPath) {
        if (!std::filesystem::is_directory(directoryPath)) return std::nullopt;

        int hash = 0;
        bool hasFile = false;
        std::error_code error_code;
        auto dir_iter = std::filesystem::directory_iterator(directoryPath, error_code);

        if (error_code) {
            WARNING("Failed to get directory iterator for directory {}: {}", directoryPath.string(), error_code.message());
            return std::nullopt;
        }

        for (auto const& entry : dir_iter) {
            if(!entry.is_directory()) {
                hasFile = true;
                hash ^= entry.file_size() ^ std::chrono::duration_cast<std::chrono::seconds>(std::filesystem::last_write_time(entry).time_since_epoch()).count();
            }
        }

        if(!hasFile)
            return std::nullopt;
        return hash;
    }
}
