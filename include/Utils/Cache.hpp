#pragma once

#include <optional>
#include <string>
#include <filesystem>
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"

#include "rust/song_loader_rs_wrapper.hpp"

namespace SongCore::Utils {

    /// @brief gets the cached info for the level
    /// @return optional song info entry, if the info isn't able to provided this returns nullopt (i.e. no song found at path)
    std::optional<LoadedSong> GetCachedInfo(std::filesystem::path const& levelPath);

    std::optional<LoadedSongs> LoadDirectory(std::filesystem::path const& directoryPath);
    std::optional<LoadedSongs> LoadDirectories(std::span<std::filesystem::path const> directoryPath);

    /// @brief just removes cached info if it exists
    void RemoveCachedInfo(std::filesystem::path const& levelPath);

    /// @brief clears all entries from song info cache
    void ClearSongInfoCache();

    /// @brief saves the current state of the cache to disk storage
    void SaveSongInfoCache();

    /// @brief loads the current state of the cache from disk storage
    /// @return boolean whether cache loaded succesfully
    bool LoadSongInfoCache();
}
