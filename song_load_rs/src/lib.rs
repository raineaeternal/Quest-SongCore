use std::{ffi::CStr, os::raw::c_void, path::Path};

use crate::{cache::SongCache, song_load::LoadedSong};

pub mod bindings;

pub mod audio_load;
pub mod hash;

pub mod beatmap;
pub mod cache;
pub mod models;
pub mod song_load;

#[repr(C)]
#[derive(Clone, Copy)]
struct OpaqueUserData(*const c_void);

unsafe impl Sync for OpaqueUserData {}
unsafe impl Send for OpaqueUserData {}

#[unsafe(no_mangle)]
pub extern "C" fn hello_from_rust() {
    #[cfg(feature = "paper2-logging")]
    paper2_tracing::init_paper_tracing(Some("song_load_rs".to_owned()))
        .expect("Failed to initialize tracing");

    tracing::info!("Hello from Rust!");
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CLoadedSong {
    pub path: *const std::os::raw::c_char,
    pub hash: *const std::os::raw::c_char,
    pub duration_secs: u64,
    pub duration_nanos: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CLoadedSongs {
    pub songs: *const CLoadedSong,
    pub count: usize,
}

impl From<song_load::LoadedSong> for CLoadedSong {
    fn from(loaded_song: song_load::LoadedSong) -> Self {
        let c_path = std::ffi::CString::new(loaded_song.path.to_str().unwrap()).unwrap();
        let c_hash = std::ffi::CString::new(loaded_song.hash).unwrap();

        CLoadedSong {
            path: c_path.into_raw(),
            hash: c_hash.into_raw(),
            duration_secs: loaded_song
                .song_length
                .map(|d| d.as_secs())
                .unwrap_or(u64::MAX),
            duration_nanos: loaded_song
                .song_length
                .map(|d| d.subsec_nanos())
                .unwrap_or(u32::MAX),
        }
    }
}

impl From<song_load::LoadedSongs> for CLoadedSongs {
    fn from(loaded_songs: song_load::LoadedSongs) -> Self {
        let c_songs: Vec<CLoadedSong> = loaded_songs
            .songs
            .into_iter()
            .map(CLoadedSong::from)
            .collect();

        let count = c_songs.len();
        let songs_ptr = c_songs.as_ptr();

        // Prevent the vector from being deallocated
        std::mem::forget(c_songs);

        CLoadedSongs {
            songs: songs_ptr,
            count,
        }
    }
}

impl From<CLoadedSong> for song_load::LoadedSong {
    fn from(c_loaded_song: CLoadedSong) -> Self {
        let path_cstr = unsafe { CStr::from_ptr(c_loaded_song.path) };
        let hash_cstr = unsafe { CStr::from_ptr(c_loaded_song.hash) };

        let path_str = path_cstr.to_str().unwrap().to_owned();
        let hash_str = hash_cstr.to_str().unwrap().to_owned();

        let duration = if c_loaded_song.duration_nanos != u32::MAX
            && c_loaded_song.duration_secs != u64::MAX
        {
            Some(std::time::Duration::new(
                c_loaded_song.duration_secs,
                c_loaded_song.duration_nanos,
            ))
        } else {
            None
        };

        song_load::LoadedSong {
            hash: hash_str,
            path: std::path::PathBuf::from(path_str),
            song_length: duration,
        }
    }
}

impl From<CLoadedSongs> for song_load::LoadedSongs {
    fn from(c_loaded_songs: CLoadedSongs) -> Self {
        let songs_slice =
            unsafe { std::slice::from_raw_parts(c_loaded_songs.songs, c_loaded_songs.count) };

        let songs: Vec<song_load::LoadedSong> = songs_slice
            .iter()
            .map(|c_song| song_load::LoadedSong::from(*c_song))
            .collect();

        song_load::LoadedSongs { songs }
    }
}

/// Represents a song cache trait for use in FFI.
#[repr(C)]
pub struct CSongCache {
    pub inner: Box<dyn SongCache>,
}

/// Loads a song from the given path (file or directory).
/// If a directory is given, it will attempt to load the song from there.
/// If a file is given, it will attempt to load the song from the zip file.
///
/// # Parameters
/// - `path`: A pointer to a null-terminated C string representing the path to the song (zip file or directory).
/// - `cache`: A pointer to a `CSongCache` struct representing the song cache (can be null to ignore cache).
///
/// # Safety
/// The `path` pointer must be a valid null-terminated C string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_loader_load_path(
    path: *const std::os::raw::c_char,
    cache: *mut CSongCache,
) -> CLoadedSong {
    if path.is_null() {
        panic!("Path is null");
    }
    let path = unsafe { CStr::from_ptr(path) }
        .to_str()
        .map(Path::new)
        .expect("Failed to convert path to str");

    let cache = unsafe { cache.as_mut().map(|c| c.inner.as_mut()) };

    let song_load =
        song_load::load_song_from_path(path.into(), cache).expect("Failed to load song from path");

    song_load.into()
}

/// Loads all songs from the given directory.
///
/// # Parameters
/// - `path`: A pointer to a null-terminated C string representing the path to the directory of songs
/// - `cache`: A pointer to a `CSongCache` instance for caching (can be null to ignore cache).
///
/// # Safety
/// The `path` pointer must be a valid null-terminated C string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_loader_load_directory(
    path: *const std::os::raw::c_char,
    cache: *mut CSongCache,
    user_data: OpaqueUserData,
    fn_callback: Option<extern "C" fn(CLoadedSong, usize, usize, OpaqueUserData)>,
) -> CLoadedSongs {
    if path.is_null() {
        panic!("Path is null");
    }
    let path = unsafe { CStr::from_ptr(path) }
        .to_str()
        .map(Path::new)
        .expect("Failed to convert path to str");

    let cache = unsafe { cache.as_mut().map(|c| c.inner.as_mut()) };
    let wrapped = fn_callback.map(|callback| {
        move |song: &LoadedSong, index, count| {
            let cloaded_song = CLoadedSong::from(song.clone());
            callback(cloaded_song, index, count, user_data);
            // from to avoid
            let _ = LoadedSong::from(cloaded_song);
        }
    });

    let songs = song_load::load_song_directory(path, cache, wrapped.as_ref())
        .expect("Failed to load song directory");

    let c_loaded_songs: CLoadedSongs = songs.into();
    c_loaded_songs
}

// TODO: Cancellable parallel version
/// Loads all songs from the given directory in parallel.
///
/// # Parameters
/// - `path`: A pointer to a null-terminated C string representing the path to the directory of songs
/// - `cache`: A pointer to a `CSongCache` instance for caching (can be null to ignore cache).
/// # Safety
/// The `path` pointer must be a valid null-terminated C string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_loader_load_directory_parallel(
    path: *const std::os::raw::c_char,
    cache: *mut CSongCache,
    user_data: OpaqueUserData,
    fn_callback: Option<extern "C" fn(CLoadedSong, usize, usize, OpaqueUserData)>,
) -> CLoadedSongs {
    if path.is_null() {
        panic!("Path is null");
    }
    let path = unsafe { CStr::from_ptr(path) }
        .to_str()
        .map(Path::new)
        .expect("Failed to convert path to str");

    let cache = unsafe { cache.as_mut().map(|c| c.inner.as_mut()) };
    let wrapped = fn_callback.map(|callback| {
        move |song: &LoadedSong, index, count| {
            let cloaded_song = CLoadedSong::from(song.clone());
            callback(cloaded_song, index, count, user_data);
            // from to avoid
            let _ = LoadedSong::from(cloaded_song);
        }
    });

    let songs = song_load::load_song_directory_parallel(&[path], cache, wrapped.as_ref())
        .expect("Failed to load song directory in parallel");

    let c_loaded_songs: CLoadedSongs = songs.into();
    c_loaded_songs
}

// TODO: Cancellable parallel version
/// Loads all songs from the given directory in parallel.
///
/// # Parameters
/// - `path`: A pointer to a null-terminated C string representing the path to the directory of songs
/// - `cache`: A pointer to a `CSongCache` instance for caching (can be null to ignore cache).
/// # Safety
/// The `path` pointer must be a valid null-terminated C string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_loader_load_directories_parallel(
    paths: *const *const std::os::raw::c_char,
    path_count: usize,
    cache: *mut CSongCache,
    user_data: OpaqueUserData,
    fn_callback: Option<extern "C" fn(CLoadedSong, usize, usize, OpaqueUserData)>,
) -> CLoadedSongs {
    if paths.is_null() {
        panic!("Path is null");
    }
    let paths = unsafe { std::slice::from_raw_parts(paths, path_count) };

    let paths = paths
        .iter()
        .map(|&p| {
            if p.is_null() {
                panic!("One of the paths is null");
            }
            unsafe { CStr::from_ptr(p) }
                .to_str()
                .map(Path::new)
                .expect("Failed to convert path to str")
        })
        .collect::<Vec<&Path>>();


    let cache = unsafe { cache.as_mut().map(|c| c.inner.as_mut()) };
    let wrapped = fn_callback.map(|callback| {
        move |song: &LoadedSong, index, count| {
            let cloaded_song = CLoadedSong::from(song.clone());
            callback(cloaded_song, index, count, user_data);
            // from to avoid
            let _ = LoadedSong::from(cloaded_song);
        }
    });

    let songs = song_load::load_song_directory_parallel(&paths, cache, wrapped.as_ref())
        .expect("Failed to load song directory in parallel");

    let c_loaded_songs: CLoadedSongs = songs.into();
    c_loaded_songs
}


/// Creates a new file based song cache and returns a pointer to it.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_loader_file_cache_new(path: *const std::os::raw::c_char) -> *mut CSongCache {
    if path.is_null() {
        panic!("Path is null");
    }
    let path = unsafe { CStr::from_ptr(path) }
        .to_str()
        .map(Path::new)
        .expect("Failed to convert path to str");

    let file_cache = crate::cache::file_cache::FileCache::new(path.into());

    let song_cache = CSongCache {
        inner: Box::new(file_cache),
    };

    Box::into_raw(Box::new(song_cache))
}

/// Reloads the cache from the source.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_loader_cache_load(cache: *mut CSongCache) {
    if cache.is_null() {
        panic!("Cache is null");
    }
    let cache = unsafe { cache.as_mut().unwrap() };

    cache
        .inner
        .reload_cache()
        .expect("Failed to reload cache");
}

/// Saves the cache to the source.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_loader_cache_save(cache: *const CSongCache) {
    if cache.is_null() {
        panic!("Cache is null");
    }
    let cache = unsafe { cache.as_ref().unwrap() };

    cache
        .inner
        .save_cache()
        .expect("Failed to reload cache");
}

/// Resets the cached data for the given song path.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_loader_cache_reset_song(
    cache: *mut CSongCache,
    song_path: *const std::os::raw::c_char,
) {
    if cache.is_null() {
        panic!("Cache is null");
    }
    let cache = unsafe { cache.as_mut().unwrap() };
    if song_path.is_null() {
        panic!("Song path is null");
    }
    let song_path = unsafe { CStr::from_ptr(song_path) }
        .to_str()
        .map(Path::new)
        .expect("Failed to convert song path to str");
    cache
        .inner
        .reset_song_cache(song_path)
        .expect("Failed to reset song cache");
}

/// Clears the entire song cache.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_loader_cache_clear(cache: *mut CSongCache) {
    if cache.is_null() {
        panic!("Cache is null");
    }
    let cache = unsafe { cache.as_mut().unwrap() };

    cache
        .inner
        .clear_cache()
        .expect("Failed to clear cache");

}

/// Checks if the cache contains a cached song for the given path.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_loader_cache_contains_cache(cache: *const CSongCache, song_path: *const std::os::raw::c_char) -> bool {
    if cache.is_null() {
        panic!("Cache is null");
    }
    let cache = unsafe { cache.as_ref().unwrap() };
    if song_path.is_null() {
        panic!("Song path is null");
    }
    let song_path = unsafe { CStr::from_ptr(song_path) }
        .to_str()
        .map(Path::new)
        .expect("Failed to convert song path to str");

    match cache.inner.get_cached_song(song_path) {
        Ok(opt) => opt.is_some(),
        Err(e) => {
            panic!("Failed to check if cache contains song: {}", e);
        }
    }
}

/// Frees the given song cache.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_loader_free_song_cache(cache: *mut CSongCache) {
    if cache.is_null() {
        return;
    }
    unsafe {
        let _ = Box::from_raw(cache);
    }
}


#[unsafe(no_mangle)]
pub extern "C" fn song_loader_free_loaded_song(loaded_song: CLoadedSong) {
    let _ = LoadedSong::from(loaded_song);
}

#[unsafe(no_mangle)]
pub extern "C" fn song_loader_free_loaded_songs(loaded_songs: CLoadedSongs) {
    let _ = song_load::LoadedSongs::from(loaded_songs);
}
