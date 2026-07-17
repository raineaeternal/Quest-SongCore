#include "Utils/Hashing.hpp"
#include "CustomJSONData.hpp"
#include "Utils/Cache.hpp"
#include "logging.hpp"
#include "song_load_rs.h"
#include <filesystem>

using namespace GlobalNamespace;

namespace SongCore::Utils {
    static std::optional<std::string> GetCustomLevelHashFromPath(std::filesystem::path const& levelPath) {
        auto start = std::chrono::high_resolution_clock::now();

        // get cached info
        auto cacheData = GetCachedInfo(levelPath);
        if(!cacheData.has_value()) return std::nullopt;

        if (cacheData->sha1.has_value()) {
            DEBUG("GetCustomLevelHash Stop Result {} from cache", *cacheData->sha1);
            return *cacheData->sha1;
        }

        char hashBuf[41]; // 40 hex characters + null terminator (SHA-1 digests are a fixed size)
        if (!song_core_get_beatmap_hash_from_path_zerocopy(levelPath.string().c_str(), hashBuf, sizeof(hashBuf))) return std::nullopt;
        std::string hashHex(hashBuf);

        cacheData->sha1 = hashHex;
        SetCachedInfo(levelPath, *cacheData);

        std::chrono::milliseconds duration = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
        DEBUG("GetCustomLevelHash Stop Result {} Time {}", hashHex, duration.count());
        return hashHex;
    }

    std::optional<std::string> GetCustomLevelHash(std::filesystem::path const& levelPath, SongCore::CustomJSONData::CustomLevelInfoSaveDataV2*) {
        return GetCustomLevelHashFromPath(levelPath);
    }

    std::optional<std::string> GetCustomLevelHash(std::filesystem::path const& levelPath, SongCore::CustomJSONData::CustomBeatmapLevelSaveDataV4*) {
        return GetCustomLevelHashFromPath(levelPath);
    }

    std::optional<int> GetDirectoryHash(std::filesystem::path const& directoryPath) {
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
