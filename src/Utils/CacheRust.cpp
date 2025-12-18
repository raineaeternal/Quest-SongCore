#include "Utils/CacheRust.hpp"
#include "Utils/Cache.hpp"
#include "Utils/Errors.hpp"
#include "Utils/Hashing.hpp"
#include "Utils/File.hpp"
#include "logging.hpp"

#include <filesystem>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <fstream>

#include "paper2_scotland2/shared/utfcpp/source/utf8.h"

namespace SongCore::Utils::Rust {


    static std::shared_mutex _cacheMutex;
    static std::filesystem::path _cachePath = "/sdcard/ModData/com.beatgames.beatsaber/Mods/SongCore/CachedSongDataRust.json";
    static SongCache _songCache = SongCache::file_cache(_cachePath);

    std::optional<LoadedSong> GetCachedInfo(std::filesystem::path const& levelPath) {
        return _songCache.load_song(levelPath.c_str());
    }


    void RemoveCachedInfo(std::filesystem::path const& levelPath) {
        std::unique_lock<std::shared_mutex> shared_lock(_cacheMutex);
        _songCache.reset_song(levelPath);
    }

    void ClearSongInfoCache() {
        std::unique_lock<std::shared_mutex> lock(_cacheMutex);
        _songCache.clear();
    }

    void SaveSongInfoCache() {
        std::shared_lock<std::shared_mutex> lock(_cacheMutex);

        _songCache.save();
    }

    bool LoadSongInfoCache() {
        // if the file doesn't exist, load should fail
        if (!std::filesystem::exists(_cachePath)) return false;

        std::unique_lock<std::shared_mutex> lock(_cacheMutex);
        _songCache.reload();

        return true;
    }
}
