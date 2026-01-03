#pragma once

#include "song_load_rs.h"
#include <filesystem>
#include <span>
#include <vector>

namespace SongCore {
/// A loaded song returned from Rust.
/// Manages the memory of the underlying CBeatmapMetadata.
struct BeatmapMetadata {
  CBeatmapMetadata song;

  // take ownership
  explicit BeatmapMetadata(const CBeatmapMetadata &song) : song(song) {}

  // copying is disabled
  BeatmapMetadata(const BeatmapMetadata &) = delete;
  BeatmapMetadata &operator=(const BeatmapMetadata &) = delete;

  // moving is enabled
  BeatmapMetadata(BeatmapMetadata &&other) noexcept : song(other.song) {
    other.song.path = nullptr;
    other.song.hash = nullptr;
    other.song.duration_secs = 0;
    other.song.duration_nanos = 0;
  }
  BeatmapMetadata &operator=(BeatmapMetadata &&other) noexcept {
    if (this == &other)
      return *this;
    // Free existing resources if necessary
    if (song.path) {
      song_core_free_loaded_song(song);
    }
    song = other.song;
    other.song.path = nullptr;
    other.song.hash = nullptr;
    other.song.duration_secs = 0;
    other.song.duration_nanos = 0;

    return *this;
  }

  ~BeatmapMetadata() {
    if (song.path) {
      song_core_free_loaded_song(song);
    }
  }

  // simple equality check based on fields
  [[nodiscard]]
  bool operator==(const BeatmapMetadata &other) const {
    return song.path == other.song.path && song.hash == other.song.hash &&
           song.duration_secs == other.song.duration_secs &&
           song.duration_nanos == other.song.duration_nanos;
  }

  [[nodiscard]]
  std::string_view get_path() const {
    return std::string_view(song.path);
  }

  [[nodiscard]]
  std::string_view get_hash() const {
    return std::string_view(song.hash);
  }

  [[nodiscard]]
  uint64_t get_duration_secs() const {
    return song.duration_secs;
  }

  [[nodiscard]]
  uint32_t get_duration_nanos() const {
    return song.duration_nanos;
  }

  [[nodiscard]]
  std::chrono::duration<float> get_duration() const {
    return std::chrono::seconds(song.duration_secs) +
           std::chrono::nanoseconds(song.duration_nanos);
  }

  [[nodiscard]]
  operator CBeatmapMetadata() const {
    return song;
  }

  [[nodiscard]]
  CBeatmapMetadata const *operator->() const {
    return &song;
  }

  [[nodiscard]]
  CBeatmapMetadata *operator->() {
    return &song;
  }
};

/// A collection of loaded songs returned from Rust.
/// Manages the memory of the underlying CBeatmapMetadataArray.
struct BeatmapMetadataArray {
  CBeatmapMetadataArray songs;

  // take ownership
  explicit BeatmapMetadataArray(const CBeatmapMetadataArray &songs)
      : songs(songs) {}

  // copying is disabled
  BeatmapMetadataArray(const BeatmapMetadataArray &) = delete;
  BeatmapMetadataArray &operator=(const BeatmapMetadataArray &) = delete;

  // moving is enabled
  BeatmapMetadataArray(BeatmapMetadataArray &&other) noexcept
      : songs(other.songs) {
    other.songs.songs = nullptr;
    other.songs.count = 0;
  }
  BeatmapMetadataArray &operator=(BeatmapMetadataArray &&other) noexcept {
    if (this == &other)
      return *this;
    // Free existing resources if necessary
    if (songs.songs) {
      song_core_free_loaded_songs(songs);
    }
    songs = other.songs;
    other.songs.songs = nullptr;
    other.songs.count = 0;

    return *this;
  }

  ~BeatmapMetadataArray() {
    if (songs.songs) {
      song_core_free_loaded_songs(songs);
    }
  }

  // simple equality check based on pointer and count
  [[nodiscard]]
  bool operator==(const BeatmapMetadataArray &other) const {
    if (songs.songs != other.songs.songs) {
      return false;
    }
    if (songs.count != other.songs.count) {
      return false;
    }

    return true;
  }

  [[nodiscard]]
  std::span<const BeatmapMetadata> as_span() const {
    return std::span<const BeatmapMetadata>(
        reinterpret_cast<const BeatmapMetadata *>(songs.songs), songs.count);
  }

  [[nodiscard]]
  std::size_t size() const {
    return songs.count;
  }

  [[nodiscard]]
  std::span<const BeatmapMetadata> operator->() const {
    return as_span();
  }
};

struct SongCache {
  CSongCache *cache;

  // take ownership
  explicit SongCache(CSongCache *cache) : cache(cache) {}

  // copying is disabled
  SongCache(const SongCache &) = delete;
  SongCache &operator=(const SongCache &) = delete;

  // moving is enabled
  SongCache(SongCache &&other) noexcept : cache(other.cache) {
    other.cache = nullptr;
  }
  SongCache &operator=(SongCache &&other) noexcept {
    if (this == &other)
      return *this;
    // Free existing resources if necessary
    if (cache) {
      song_core_free_song_cache(cache);
    }
    cache = other.cache;
    other.cache = nullptr;

    return *this;
  }

  ~SongCache() {
    if (cache) {
      song_core_free_song_cache(cache);
    }
  }

  /// Creates a new file-based song cache at the given path.
  /// Does not load the cache from disk; call `reload` to do so.
  [[nodiscard]]
  static SongCache file_cache(std::filesystem::path const &cache_path) {
    CSongCache *c_cache = song_core_file_cache_new(cache_path.c_str());
    return SongCache(c_cache);
  }

  /// Reloads the cache from disk.
  void reload() { song_core_cache_load(cache); }

  /// Saves the cache to disk.
  void save() const { song_core_cache_save(cache); }

  void reset_song(std::filesystem::path const &path) {
    song_core_cache_reset_song(cache, path.c_str());
  }

  void clear() { song_core_cache_clear(cache); }

  /// Checks if the cache contains an entry for the given path.
  [[nodiscard]]
  bool contains(std::filesystem::path const &path) const {
    CBeatmapMetadata c_song = song_core_load_path(path.c_str(), cache);
    bool exists = c_song.path != nullptr;
    song_core_free_loaded_song(c_song);
    return exists;
  }

  /// Loads the cached song data for the given path.
  /// If the song is not cached, loads and caches it.
  [[nodiscard]]
  BeatmapMetadata load_song(std::filesystem::path const &path) {
    CBeatmapMetadata c_song = song_core_load_path(path.c_str(), cache);
    return BeatmapMetadata(c_song);
  }


  /// Loads all songs from the given directory, using the cache.
  /// If a song is not cached, loads and caches it.
  [[nodiscard]]
  BeatmapMetadataArray
  from_directory(std::filesystem::path const &path,
                 void (*fn_callback)(CBeatmapMetadata, uintptr_t, uintptr_t,
                                     OpaqueUserData) = nullptr,
                 void *user_data = nullptr) {
    CBeatmapMetadataArray c_songs = song_core_load_directory(
        path.c_str(), cache, OpaqueUserData{user_data}, fn_callback);
    return BeatmapMetadataArray(c_songs);
  }

  /// Loads all songs from the given directory in parallel, using the cache.
  /// If a song is not cached, loads and caches it.
  [[nodiscard]]
  BeatmapMetadataArray
  from_directory_parallel(std::filesystem::path const &path,
                          void (*fn_callback)(CBeatmapMetadata, uintptr_t,
                                              uintptr_t,
                                              OpaqueUserData) = nullptr,
                          void *user_data = nullptr) {
    CBeatmapMetadataArray c_songs = song_core_load_directory_parallel(
        path.c_str(), cache, OpaqueUserData{user_data}, fn_callback);
    return BeatmapMetadataArray(c_songs);
  }

  [[nodiscard]]
  operator CSongCache *() const {
    return cache;
  }
};

// C++ wrappers for CustomBeatmapLevel returned from Rust
struct CustomBeatmapLevelOwned {
  CCustomBeatmapLevel *level;

  explicit CustomBeatmapLevelOwned(CCustomBeatmapLevel *p) : level(p) {}

  // non-copyable
  CustomBeatmapLevelOwned(const CustomBeatmapLevelOwned &) = delete;
  CustomBeatmapLevelOwned &operator=(const CustomBeatmapLevelOwned &) = delete;

  // movable
  CustomBeatmapLevelOwned(CustomBeatmapLevelOwned &&other) noexcept
      : level(other.level) {
    other.level = nullptr;
  }
  CustomBeatmapLevelOwned &operator=(CustomBeatmapLevelOwned &&other) noexcept {
    if (this == &other)
      return *this;
    if (level) {
      song_core_free_level(level);
    }
    level = other.level;
    other.level = nullptr;
    return *this;
  }

  ~CustomBeatmapLevelOwned() {
    if (level) {
      song_core_free_level(level);
    }
  }

  [[nodiscard]]
  explicit operator bool() const {
    return level != nullptr;
  }

  [[nodiscard]]
  CCustomBeatmapLevel *get() const {
    return level;
  }

  [[nodiscard]]
  uint32_t version() const {
    return level->version;
  }

  [[nodiscard]]
  std::string_view level_id() const {
    return std::string_view(level->level_id._0);
  }

  [[nodiscard]]
  std::string_view song_name() const {
    return std::string_view(level->song_name._0);
  }

  [[nodiscard]]
  std::vector<std::string> all_mappers() const {
    std::vector<std::string> mappers;
    for (size_t i = 0; i < level->all_mappers.length; ++i) {
      mappers.emplace_back(level->all_mappers.data[i]._0);
    }
    return mappers;
  }

  [[nodiscard]]
  float beats_per_minute() const {
    return level->beats_per_minute;
  }

  [[nodiscard]]
  float song_duration() const {
    return level->song_duration;
  }

  [[nodiscard]]
  std::filesystem::path custom_level_path() const {
    return std::filesystem::path(level->custom_level_path._0);
  }

  static CustomBeatmapLevelOwned
  load_from_path(std::filesystem::path const &path, SongCache const &cache,
                 bool wip = false) {
    CCustomBeatmapLevel *p = song_core_load_level_path(path.c_str(), cache, wip);
    return CustomBeatmapLevelOwned(p);
  }

  
};

struct CustomBeatmapLevelArray {
  ManagedArray<CCustomBeatmapLevel> *levels;

  explicit CustomBeatmapLevelArray(ManagedArray<CCustomBeatmapLevel> *p)
      : levels(p) {}

  // non-copyable
  CustomBeatmapLevelArray(const CustomBeatmapLevelArray &) = delete;
  CustomBeatmapLevelArray &operator=(const CustomBeatmapLevelArray &) = delete;

  // movable
  CustomBeatmapLevelArray(CustomBeatmapLevelArray &&other) noexcept
      : levels(other.levels) {
    other.levels = nullptr;
  }
  CustomBeatmapLevelArray &operator=(CustomBeatmapLevelArray &&other) noexcept {
    if (this == &other)
      return *this;
    if (levels) {
      song_core_free_level_array(levels);
    }
    levels = other.levels;
    other.levels = nullptr;
    return *this;
  }

  ~CustomBeatmapLevelArray() {
    if (levels) {
      song_core_free_level_array(levels);
    }
  }

  [[nodiscard]]
  std::size_t size() const {
    return levels ? levels->length : 0;
  }

  [[nodiscard]]
  bool empty() const {
    return size() == 0;
  }

  std::span<const CustomBeatmapLevelOwned> as_span() const {
    if (!levels) {
      return std::span<const CustomBeatmapLevelOwned>();
    }
    return std::span<const CustomBeatmapLevelOwned>(reinterpret_cast<CustomBeatmapLevelOwned*>(levels->data), levels->length);
  }

  [[nodiscard]]
  const CCustomBeatmapLevel *data() const {
    return levels ? levels->data : nullptr;
  }

  [[nodiscard]]
  static CustomBeatmapLevelArray
  load_from_directories(std::span<const std::filesystem::path> paths,
                        SongCache const &cache, bool wip = false) {
    // Build an array of C strings from the filesystem::path span so we can pass
    // a const char** to the C API without casting away qualifiers.
    std::vector<const char *> c_paths;
    c_paths.reserve(paths.size());
    for (auto const &p : paths) {
      c_paths.push_back(p.c_str());
    }

    ManagedArray<CCustomBeatmapLevel> *c_levels =
        song_core_load_level_from_directories(c_paths.data(), paths.size(),
                                              cache, wip);

    return CustomBeatmapLevelArray(c_levels);
  }

  using CallbackType =
      std::function<void(CCustomBeatmapLevel const &, size_t, size_t)>;

  [[nodiscard]]
  static CustomBeatmapLevelArray
  load_from_directories_parallel(std::span<const std::filesystem::path> paths,
                                 SongCache const &cache, bool wip,
                                 CallbackType callback = nullptr) {
    // Build an array of C strings from the filesystem::path span so we can pass
    // a const char** to the C API without casting away qualifiers.
    std::vector<const char *> c_paths;
    c_paths.reserve(paths.size());
    for (auto const &p : paths) {
      c_paths.push_back(p.c_str());
    }

    // Wrap the callback to match the C function signature
    auto c_cb = +[](const CCustomBeatmapLevel *c_level, uintptr_t index,
                    uintptr_t total, OpaqueUserData user_data) {
      auto *cb_ptr = reinterpret_cast<CallbackType const *>(user_data._0);

      if (!cb_ptr) {
        return;
      }

      (*cb_ptr)(*c_level, static_cast<size_t>(index),
                static_cast<size_t>(total));
    };

    ManagedArray<CCustomBeatmapLevel> *c_levels =
        song_core_load_levels_from_directories_parallel(
            c_paths.data(), paths.size(), cache, wip, OpaqueUserData{&callback},
            c_cb);

    return CustomBeatmapLevelArray(c_levels);
  }
};

} // namespace SongCore