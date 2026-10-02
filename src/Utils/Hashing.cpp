#include "Utils/Hashing.hpp"
#include "Utils/Sha1Neon.hpp"
#include "CustomJSONData.hpp"
#include "Utils/Cache.hpp"
#include "logging.hpp"
#include <filesystem>

using namespace GlobalNamespace;

namespace SongCore::Utils {
    std::optional<std::string> GetCustomLevelHash(std::filesystem::path const& levelPath, SongCore::CustomJSONData::CustomLevelInfoSaveDataV2* saveData) {
        auto start = std::chrono::high_resolution_clock::now();

        // get cached info
        auto cacheData = GetCachedInfo(levelPath);
        if(!cacheData.has_value()) return std::nullopt;

        if (cacheData->sha1.has_value()) {
            DEBUG("GetCustomLevelHash Stop Result {} from cache", *cacheData->sha1);
            return *cacheData->sha1;
        }

        auto infoPath = levelPath / "info.dat";
        if(!std::filesystem::exists(infoPath)) {
            infoPath = levelPath / "Info.dat";
            if(!std::filesystem::exists(infoPath)) return std::nullopt;
        }

        auto sha1Neon = SHA1_NEON();
        sha1Neon.updateFile(infoPath);

        for(auto val : saveData->difficultyBeatmapSets) {
            if (!val) continue;
            auto difficultyBeatmaps = val->difficultyBeatmaps;
            if (!difficultyBeatmaps) continue;
            for(auto difficultyBeatmap : difficultyBeatmaps) {
                auto diffPath = levelPath / static_cast<std::string>(difficultyBeatmap->beatmapFilename);
                if(!std::filesystem::exists(diffPath)) {
                    ERROR("GetCustomLevelHash File {} did not exist", diffPath.string());
                    continue;
                }
                sha1Neon.updateFile(diffPath);
            }
        }

        cacheData->sha1 = sha1Neon.finalize();
        SetCachedInfo(levelPath, *cacheData);

        std::chrono::milliseconds duration = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
        DEBUG("GetCustomLevelHash Stop Result {} Time {}", *cacheData->sha1, duration.count());
        return cacheData->sha1;
    }

    std::optional<std::string> GetCustomLevelHash(std::filesystem::path const& levelPath, SongCore::CustomJSONData::CustomBeatmapLevelSaveDataV4* saveData) {
        auto start = std::chrono::high_resolution_clock::now();

        // get cached info
        auto cacheData = GetCachedInfo(levelPath);
        if(!cacheData.has_value()) return std::nullopt;

        if (cacheData->sha1.has_value()) {
            DEBUG("GetCustomLevelHash Stop Result {} from cache", *cacheData->sha1);
            return *cacheData->sha1;
        }

        auto infoPath = levelPath / "info.dat";
        if(!std::filesystem::exists(infoPath)) {
            infoPath = levelPath / "Info.dat";
            if(!std::filesystem::exists(infoPath)) return std::nullopt;
        }

        auto audioPath = levelPath / static_cast<std::string>(saveData->audio.audioDataFilename);
        if(!std::filesystem::exists(audioPath)) {
            return std::nullopt;
        }

        auto sha1Neon = SHA1_NEON();
        sha1Neon.updateFile(infoPath);
        sha1Neon.updateFile(audioPath);

        for(auto val : saveData->difficultyBeatmaps) {
            if (!val) continue;
            
            auto diffPath = levelPath / static_cast<std::string>(val->beatmapDataFilename);
            if(!std::filesystem::exists(diffPath)) {
                ERROR("GetCustomLevelHash File {} did not exist", diffPath.string());
                continue;
            }
            sha1Neon.updateFile(diffPath);

            auto lightPath = levelPath / static_cast<std::string>(val->lightshowDataFilename);
            if(!std::filesystem::exists(lightPath)) {
                ERROR("GetCustomLevelHash Lighting File {} did not exist", diffPath.string());
                continue;
            }
            sha1Neon.updateFile(lightPath);
        }

        cacheData->sha1 = sha1Neon.finalize();
        SetCachedInfo(levelPath, *cacheData);

        std::chrono::milliseconds duration = duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
        DEBUG("GetCustomLevelHash Stop Result {} Time {}", *cacheData->sha1, duration.count());
        return cacheData->sha1;
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
