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
    return song.path == other.song.path &&
           song.hash == other.song.hash &&
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
  explicit BeatmapMetadataArray(const CBeatmapMetadataArray &songs) : songs(songs) {}

  // copying is disabled
  BeatmapMetadataArray(const BeatmapMetadataArray &) = delete;
  BeatmapMetadataArray &operator=(const BeatmapMetadataArray &) = delete;

  // moving is enabled
  BeatmapMetadataArray(BeatmapMetadataArray &&other) noexcept : songs(other.songs) {
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
  static SongCache file_cache(std::filesystem::path const& cache_path) {
    CSongCache* c_cache = song_core_file_cache_new(cache_path.c_str());
    return SongCache(c_cache);
  }

  /// Reloads the cache from disk.
  void reload() {
    song_core_cache_load(cache);
  }

  /// Saves the cache to disk.
  void save() const {
    song_core_cache_save(cache);
  }

  void reset_song(std::filesystem::path const& path) {
    song_core_cache_reset_song(cache, path.c_str());
  }

  void clear() {
    song_core_cache_clear(cache);
  }

  
  /// Checks if the cache contains an entry for the given path.
  [[nodiscard]]
  bool contains(std::filesystem::path const& path) const {
    CBeatmapMetadata c_song = song_core_load_path(path.c_str(), cache);
    bool exists = c_song.path != nullptr;
    song_core_free_loaded_song(c_song);
    return exists;
  }

  /// Loads the cached song data for the given path.
  /// If the song is not cached, loads and caches it.
  [[nodiscard]]
  BeatmapMetadata load_song(std::filesystem::path const& path) {
    CBeatmapMetadata c_song = song_core_load_path(path.c_str(), cache);
    return BeatmapMetadata(c_song);
  }

  /// Loads all songs from the given directory, using the cache.
  /// If a song is not cached, loads and caches it.
  [[nodiscard]]
  BeatmapMetadataArray from_directory(std::filesystem::path const& path,
                                       void (*fn_callback)(CBeatmapMetadata,
                                                            uintptr_t,
                                                            uintptr_t,
                                                            OpaqueUserData) = nullptr, void* user_data = nullptr) {
    CBeatmapMetadataArray c_songs = song_core_load_directory(path.c_str(), cache, OpaqueUserData{user_data}, fn_callback);
    return BeatmapMetadataArray(c_songs);
  }


  [[nodiscard]]
  BeatmapMetadataArray from_directory_parallel(std::span<const std::filesystem::path> paths,
                                       void (*fn_callback)(CBeatmapMetadata,
                                                            uintptr_t,
                                                            uintptr_t,
                                                            OpaqueUserData) = nullptr, void* user_data = nullptr) {
    // Build an array of C strings from the filesystem::path span so we can pass
    // a const char** to the C API without casting away qualifiers.
    std::vector<const char*> c_paths;
    c_paths.reserve(paths.size());
    for (auto const& p : paths) {
      c_paths.push_back(p.c_str());
    }

    CBeatmapMetadataArray c_songs = song_core_load_directories_parallel(
        c_paths.data(),
        paths.size(),
        cache,
        OpaqueUserData{user_data},
        fn_callback
    );
    return BeatmapMetadataArray(c_songs);
  }

  /// Loads all songs from the given directory in parallel, using the cache.
  /// If a song is not cached, loads and caches it.
  [[nodiscard]]
  BeatmapMetadataArray from_directory_parallel(std::filesystem::path const& path, void (*fn_callback)(CBeatmapMetadata,
                                                            uintptr_t,
                                                            uintptr_t,
                                                            OpaqueUserData) = nullptr, void* user_data = nullptr) {
    CBeatmapMetadataArray c_songs = song_core_load_directory_parallel(path.c_str(), cache, OpaqueUserData{user_data}, fn_callback);
    return BeatmapMetadataArray(c_songs);
  }


  [[nodiscard]]
  operator CSongCache *() const {
    return cache;
  }
};


} // namespace SongCore