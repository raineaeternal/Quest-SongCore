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
    static std::filesystem::path _cachePath = "/sdcard/ModData/com.beatgames.beatsaber/Mods/SongCore/CachedSongDataRust.json";
    
    // will not load until reload is called
    static SongCache _songCache = SongCache::file_cache(_cachePath);


    void RemoveCachedInfo(std::filesystem::path const& levelPath) {
        _songCache.reset_song(levelPath);
    }

    void ClearSongInfoCache() {
        _songCache.clear();
    }

    std::optional<BeatmapMetadata>
    GetCachedInfo(std::filesystem::path const &levelPath) {
      if (!std::filesystem::exists(levelPath)) {
        return std::nullopt;
      }

      if (_songCache.contains(levelPath))
        return _songCache.load_metadata(levelPath.c_str());

      return _songCache.load_metadata(levelPath);
    }

    std::optional<BeatmapMetadataArray>
    LoadDirectory(std::filesystem::path const &directoryPath) {
      if (!std::filesystem::exists(directoryPath)) {
        return std::nullopt;
      }

      return _songCache.metadata_of_directory_parallel(directoryPath);
    }
    
    BeatmapMetadataArray
    LoadDirectories(std::span<std::filesystem::path const> directoryPath) {
      return _songCache.metadata_of_directories_parallel(directoryPath);
    }

    void SaveSongInfoCache() {
        _songCache.save();
    }

    bool LoadSongInfoCache() {
        // if the file doesn't exist, load should fail
        if (!std::filesystem::exists(_cachePath)) return false;

        _songCache.reload();

        return true;
    }
}
