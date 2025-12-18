#include "song_load_rs.h"
#include <filesystem>
#include <span>

namespace SongCore {
/// A loaded song returned from Rust.
/// Manages the memory of the underlying CLoadedSong.
struct LoadedSong {
  CLoadedSong song;

  // take ownership
  explicit LoadedSong(const CLoadedSong &song) : song(song) {}

  // copying is disabled
  LoadedSong(const LoadedSong &) = delete;
  LoadedSong &operator=(const LoadedSong &) = delete;

  // moving is enabled
  LoadedSong(LoadedSong &&other) noexcept : song(other.song) {
    other.song.path = nullptr;
    other.song.hash = nullptr;
    other.song.duration_secs = 0;
    other.song.duration_nanos = 0;
  }
  LoadedSong &operator=(LoadedSong &&other) noexcept {
    if (this == &other)
      return *this;
    // Free existing resources if necessary
    if (song.path) {
      song_loader_free_loaded_song(song);
    }
    song = other.song;
    other.song.path = nullptr;
    other.song.hash = nullptr;
    other.song.duration_secs = 0;
    other.song.duration_nanos = 0;

    return *this;
  }

  ~LoadedSong() {
    if (song.path) {
      song_loader_free_loaded_song(song);
    }
  }

  // simple equality check based on fields
  [[nodiscard]]
  bool operator==(const LoadedSong &other) const {
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
  operator CLoadedSong() const {
    return song;
  }

  [[nodiscard]]
  CLoadedSong const *operator->() const {
    return &song;
  }

  [[nodiscard]]
  CLoadedSong *operator->() {
    return &song;
  }
};

/// A collection of loaded songs returned from Rust.
/// Manages the memory of the underlying CLoadedSongs.
struct LoadedSongs {
  CLoadedSongs songs;

  // take ownership
  explicit LoadedSongs(const CLoadedSongs &songs) : songs(songs) {}

  // copying is disabled
  LoadedSongs(const LoadedSongs &) = delete;
  LoadedSongs &operator=(const LoadedSongs &) = delete;

  // moving is enabled
  LoadedSongs(LoadedSongs &&other) noexcept : songs(other.songs) {
    other.songs.songs = nullptr;
    other.songs.count = 0;
  }
  LoadedSongs &operator=(LoadedSongs &&other) noexcept {
    if (this == &other)
      return *this;
    // Free existing resources if necessary
    if (songs.songs) {
      song_loader_free_loaded_songs(songs);
    }
    songs = other.songs;
    other.songs.songs = nullptr;
    other.songs.count = 0;

    return *this;
  }

  ~LoadedSongs() {
    if (songs.songs) {
      song_loader_free_loaded_songs(songs);
    }
  }

  // simple equality check based on pointer and count
  [[nodiscard]]
  bool operator==(const LoadedSongs &other) const {
    if (songs.songs != other.songs.songs) {
      return false;
    }
    if (songs.count != other.songs.count) {
      return false;
    }

    return true;
  }


  [[nodiscard]]
  std::span<const LoadedSong> as_span() const {
    return std::span<const LoadedSong>(
        reinterpret_cast<const LoadedSong *>(songs.songs), songs.count);
  }

  [[nodiscard]]
  std::span<const LoadedSong> operator->() const {
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
      song_loader_free_song_cache(cache);
    }
    cache = other.cache;
    other.cache = nullptr;

    return *this;
  }

  ~SongCache() {
    if (cache) {
      song_loader_free_song_cache(cache);
    }
  }

  /// Creates a new file-based song cache at the given path.
  /// Does not load the cache from disk; call `reload` to do so.
  [[nodiscard]]
  static SongCache file_cache(std::filesystem::path const& cache_path) {
    CSongCache* c_cache = song_loader_file_cache_new(cache_path.c_str());
    return SongCache(c_cache);
  }

  /// Reloads the cache from disk.
  void reload() {
    song_loader_cache_load(cache);
  }

  /// Saves the cache to disk.
  void save() const {
    song_loader_cache_save(cache);
  }

  void reset_song(std::filesystem::path const& path) {
    song_loader_cache_reset_song(cache, path.c_str());
  }

  void clear() {
    song_loader_cache_clear(cache);
  }

  
  /// Checks if the cache contains an entry for the given path.
  [[nodiscard]]
  bool contains(std::filesystem::path const& path) const {
    CLoadedSong c_song = song_loader_load_path(path.c_str(), cache);
    bool exists = c_song.path != nullptr;
    song_loader_free_loaded_song(c_song);
    return exists;
  }

  /// Loads the cached song data for the given path.
  /// If the song is not cached, loads and caches it.
  [[nodiscard]]
  LoadedSong load_song(std::filesystem::path const& path) {
    CLoadedSong c_song = song_loader_load_path(path.c_str(), cache);
    return LoadedSong(c_song);
  }

  /// Loads all songs from the given directory, using the cache.
  /// If a song is not cached, loads and caches it.
  [[nodiscard]]
  LoadedSongs from_directory(std::filesystem::path const& path,
                                       void (*fn_callback)(CLoadedSong,
                                                            uintptr_t,
                                                            uintptr_t,
                                                            OpaqueUserData) = nullptr, void* user_data = nullptr) {
    CLoadedSongs c_songs = song_loader_load_directory(path.c_str(), cache, OpaqueUserData{user_data}, fn_callback);
    return LoadedSongs(c_songs);
  }

  /// Loads all songs from the given directory in parallel, using the cache.
  /// If a song is not cached, loads and caches it.
  [[nodiscard]]
  LoadedSongs from_directory_parallel(std::filesystem::path const& path, void (*fn_callback)(CLoadedSong,
                                                            uintptr_t,
                                                            uintptr_t,
                                                            OpaqueUserData) = nullptr, void* user_data = nullptr) {
    CLoadedSongs c_songs = song_loader_load_directory_parallel(path.c_str(), cache, OpaqueUserData{user_data}, fn_callback);
    return LoadedSongs(c_songs);
  }


  [[nodiscard]]
  operator CSongCache *() const {
    return cache;
  }
};
} // namespace SongCore