#include "Utils/Cache.hpp"
#include "Utils/Errors.hpp"
#include "Utils/File.hpp"
#include "logging.hpp"

#include <filesystem>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <fstream>

#include "paper2_scotland2/shared/utfcpp/source/utf8.h"

namespace SongCore::Utils {
    static std::shared_mutex _cacheMutex;
    static std::filesystem::path _cachePath = "/sdcard/ModData/com.beatgames.beatsaber/Mods/SongCore/CachedSongDataRust.json";
    
    // will not load until reload is called
    static SongCache _songCache = SongCache::file_cache(_cachePath);

    SongCache& GetSongCache() {
        return _songCache;
    }

    void RemoveCachedInfo(std::filesystem::path const& levelPath) {
        std::unique_lock<std::shared_mutex> shared_lock(_cacheMutex);
        _songCache.reset_song(levelPath);
    }

    void ClearSongInfoCache() {
        std::unique_lock<std::shared_mutex> lock(_cacheMutex);
        _songCache.clear();
    }

    std::optional<LoadedSong> GetCachedInfo(std::filesystem::path const &levelPath) {
        if (!std::filesystem::exists(levelPath)) {
            return std::nullopt;
        }

        // first check if it exists
      {
        std::shared_lock<std::shared_mutex> shared_lock(_cacheMutex);

        if (_songCache.contains(levelPath))
          return _songCache.load_song(levelPath.c_str());
      }

      // now try to load it
      std::unique_lock<std::shared_mutex> shared_lock(_cacheMutex);
      return _songCache.load_song(levelPath);
    }

    std::optional<LoadedSongs> LoadDirectory(std::filesystem::path const& directoryPath) {
        if (!std::filesystem::exists(directoryPath)) {
            return std::nullopt;
        }

        std::unique_lock<std::shared_mutex> shared_lock(_cacheMutex);
        return _songCache.from_directory_parallel(directoryPath);
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
