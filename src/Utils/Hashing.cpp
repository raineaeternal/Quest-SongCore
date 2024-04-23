#include "Utils/Hashing.hpp"
#include "CustomJSONData.hpp"
#include "Utils/Cache.hpp"
#include "logging.hpp"
#include <filesystem>

#include "libcryptopp/shared/sha.h"
#include "libcryptopp/shared/hex.h"
#include "libcryptopp/shared/files.h"

using namespace GlobalNamespace;
using namespace CryptoPP;

namespace SongCore::Utils {
    // v2 savedata
    std::vector<std::filesystem::path> GetFilesForHashing(std::filesystem::path const& levelPath, SongCore::CustomJSONData::CustomLevelInfoSaveData* saveData) {
        auto infoPath = levelPath / "info.dat";
        if(!std::filesystem::exists(infoPath)) {
            infoPath = levelPath / "Info.dat";
            if(!std::filesystem::exists(infoPath)) return {};
        }

        std::vector<std::filesystem::path> files;

        files.emplace_back(infoPath);

        for (auto characteristic : saveData->difficultyBeatmapSets) {
            if (!characteristic) continue;
            auto diffs = characteristic->difficultyBeatmaps;
            if (!diffs) continue;
            for (auto diff : diffs) {
                if (!diff) continue;
                auto diffPath = levelPath / static_cast<std::string>(diff->beatmapFilename);
                if (!std::filesystem::exists(diffPath)) continue;

                files.emplace_back(diffPath);
            }
        }

        return files;
    }

    // v4 savedata
    std::vector<std::filesystem::path> GetFilesForHashing(std::filesystem::path const& levelPath, SongCore::CustomJSONData::CustomBeatmapLevelSaveData* saveData) {
        auto infoPath = levelPath / "info.dat";
        if(!std::filesystem::exists(infoPath)) {
            infoPath = levelPath / "Info.dat";
            if(!std::filesystem::exists(infoPath)) return {};
        }

        auto audioPath = levelPath / static_cast<std::string>(saveData->audio.audioDataFilename);
        if(!std::filesystem::exists(infoPath)) return {};

        std::vector<std::filesystem::path> files;

        files.emplace_back(infoPath);
        files.emplace_back(audioPath);

        for (auto diff : saveData->difficultyBeatmaps) {
            if (!diff) continue;
            auto diffPath = levelPath / static_cast<std::string>(diff->beatmapDataFilename);
            auto lightPath = levelPath / static_cast<std::string>(diff->lightshowDataFilename);

            if (!std::filesystem::exists(diffPath)) continue;
            if (!std::filesystem::exists(lightPath)) continue;

            files.emplace_back(diffPath);
            files.emplace_back(lightPath);
        }

        return files;
    }

    /// @brief templated hash calculation to not repeat code. we always want to get the files and hash them in order
    template<typename T>
    std::optional<std::string> CalculateHash(std::filesystem::path const& levelPath, T saveData) {
        auto start = std::chrono::high_resolution_clock::now();
        auto filesToHash = GetFilesForHashing(levelPath, saveData);
        // if we found no files, return nullopt
        if (filesToHash.empty()) return std::nullopt;

        SHA1 hashType;
        std::string hashResult;
        HashFilter hashFilter(hashType, new StringSink(hashResult));

        for (auto const& file : filesToHash) {
            FileSource fs(file.c_str(), false);
            fs.Attach(new Redirector(hashFilter));
            fs.Pump(LWORD_MAX);
            fs.Detach();
        }

        hashFilter.MessageEnd();

        std::string hashHex;
        HexEncoder hexEncoder(new StringSink(hashHex));
        hexEncoder.Put((const byte*)hashResult.data(), hashResult.size());

        DEBUG("Calculated hash in {}ms", duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count());
        return hashHex;
    }

    template<typename T>
    std::optional<std::string> GetCustomLevelHashInternal(std::filesystem::path const& levelPath, T saveData) {
        // get cached info
        auto cacheData = GetCachedInfo(levelPath);
        if(!cacheData.has_value()) return std::nullopt;

        if (cacheData->sha1.has_value()) {
            DEBUG("GetCustomLevelHash Stop Result {} from cache", *cacheData->sha1);
            return *cacheData->sha1;
        }

        // calculate the hash
        auto calculatedHash = CalculateHash(levelPath, saveData);

        // we failed
        if (!calculatedHash.has_value()) {
            DEBUG("GetCustomLevelHash Stop Failed to calculate hash");
            return std::nullopt;
        }

        auto& hash = calculatedHash.value();
        cacheData->sha1 = hash;
        SetCachedInfo(levelPath, *cacheData);

        DEBUG("GetCustomLevelHash Stop Result {}", hash);
        return calculatedHash;
    }

    std::optional<std::string> GetCustomLevelHash(std::filesystem::path const& levelPath, SongCore::CustomJSONData::CustomLevelInfoSaveData* saveData) {
        return GetCustomLevelHashInternal(levelPath, saveData);
    }

    std::optional<std::string> GetCustomLevelHash(std::filesystem::path const& levelPath, SongCore::CustomJSONData::CustomBeatmapLevelSaveData* saveData) {
        return GetCustomLevelHashInternal(levelPath, saveData);
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
