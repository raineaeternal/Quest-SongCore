use std::{ffi::CStr, path::Path};

use crate::{
    ffi::{OpaqueUserData, cache::CSongCache},
    loader::song_data_loader::{
        SongCacheData, SongCacheDatas, load_song_directory, load_song_directory_parallel,
        load_song_from_path,
    },
};

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CSongCacheData {
    pub path: *const std::os::raw::c_char,
    pub hash: *const std::os::raw::c_char,
    pub duration_secs: u64,
    pub duration_nanos: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CSongCacheDatas {
    pub songs: *const CSongCacheData,
    pub count: usize,
}

impl From<SongCacheData> for CSongCacheData {
    fn from(loaded_song: SongCacheData) -> Self {
        let c_path = std::ffi::CString::new(loaded_song.path.to_str().unwrap()).unwrap();
        let c_hash = std::ffi::CString::new(loaded_song.hash).unwrap();

        CSongCacheData {
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

impl From<SongCacheDatas> for CSongCacheDatas {
    fn from(loaded_songs: SongCacheDatas) -> Self {
        let c_songs: Vec<CSongCacheData> = loaded_songs
            .songs
            .into_iter()
            .map(CSongCacheData::from)
            .collect();

        let count = c_songs.len();
        let songs_ptr = c_songs.as_ptr();

        // Prevent the vector from being deallocated
        std::mem::forget(c_songs);

        CSongCacheDatas {
            songs: songs_ptr,
            count,
        }
    }
}

impl From<CSongCacheData> for SongCacheData {
    fn from(c_loaded_song: CSongCacheData) -> Self {
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

        SongCacheData {
            hash: hash_str,
            path: std::path::PathBuf::from(path_str),
            song_length: duration,
        }
    }
}

impl From<CSongCacheDatas> for SongCacheDatas {
    fn from(c_loaded_songs: CSongCacheDatas) -> Self {
        let songs_slice =
            unsafe { std::slice::from_raw_parts(c_loaded_songs.songs, c_loaded_songs.count) };

        let songs: Vec<SongCacheData> = songs_slice
            .iter()
            .map(|c_song| SongCacheData::from(*c_song))
            .collect();

        SongCacheDatas { songs }
    }
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
) -> CSongCacheData {
    if path.is_null() {
        panic!("Path is null");
    }
    let path = unsafe { CStr::from_ptr(path) }
        .to_str()
        .map(Path::new)
        .expect("Failed to convert path to str");

    let cache = unsafe { cache.as_mut().map(|c| c.inner.as_mut()) };

    let song_load = load_song_from_path(path.into(), cache).expect("Failed to load song from path");

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
    fn_callback: Option<extern "C" fn(CSongCacheData, usize, usize, OpaqueUserData)>,
) -> CSongCacheDatas {
    if path.is_null() {
        panic!("Path is null");
    }
    let path = unsafe { CStr::from_ptr(path) }
        .to_str()
        .map(Path::new)
        .expect("Failed to convert path to str");

    let cache = unsafe { cache.as_mut().map(|c| c.inner.as_mut()) };
    let wrapped = fn_callback.map(|callback| {
        move |song: &SongCacheData, index, count| {
            let cloaded_song = CSongCacheData::from(song.clone());
            callback(cloaded_song, index, count, user_data);
            // from to avoid
            let _ = SongCacheData::from(cloaded_song);
        }
    });

    let songs =
        load_song_directory(path, cache, wrapped.as_ref()).expect("Failed to load song directory");

    let c_loaded_songs: CSongCacheDatas = songs.into();
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
    fn_callback: Option<extern "C" fn(CSongCacheData, usize, usize, OpaqueUserData)>,
) -> CSongCacheDatas {
    if path.is_null() {
        panic!("Path is null");
    }
    let path = unsafe { CStr::from_ptr(path) }
        .to_str()
        .map(Path::new)
        .expect("Failed to convert path to str");

    let cache = unsafe { cache.as_mut().map(|c| c.inner.as_mut()) };
    let wrapped = fn_callback.map(|callback| {
        move |song: &SongCacheData, index, count| {
            let cloaded_song = CSongCacheData::from(song.clone());
            callback(cloaded_song, index, count, user_data);
            // from to avoid
            let _ = SongCacheData::from(cloaded_song);
        }
    });

    let songs = load_song_directory_parallel(&[path], cache, wrapped.as_ref())
        .expect("Failed to load song directory in parallel");

    let c_loaded_songs: CSongCacheDatas = songs.into();
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
    fn_callback: Option<extern "C" fn(CSongCacheData, usize, usize, OpaqueUserData)>,
) -> CSongCacheDatas {
    if paths.is_null() {
        panic!("Path is null");
    }
    let paths = unsafe { std::slice::from_raw_parts(paths, path_count) };

    let paths: Vec<&Path> = paths
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
        .collect();

    let cache = unsafe { cache.as_mut().map(|c| c.inner.as_mut()) };
    let wrapped = fn_callback.map(|callback| {
        move |song: &SongCacheData, index, count| {
            let cloaded_song = CSongCacheData::from(song.clone());
            callback(cloaded_song, index, count, user_data);
            // from to avoid
            let _ = SongCacheData::from(cloaded_song);
        }
    });

    let songs = load_song_directory_parallel(&paths, cache, wrapped.as_ref())
        .expect("Failed to load song directory in parallel");

    let c_loaded_songs: CSongCacheDatas = songs.into();
    c_loaded_songs
}

#[unsafe(no_mangle)]
pub extern "C" fn song_loader_free_loaded_song(loaded_song: CSongCacheData) {
    let _ = SongCacheData::from(loaded_song);
}

#[unsafe(no_mangle)]
pub extern "C" fn song_loader_free_loaded_songs(loaded_songs: CSongCacheDatas) {
    let _ = SongCacheDatas::from(loaded_songs);
}
