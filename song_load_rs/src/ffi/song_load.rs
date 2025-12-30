use std::{ffi::CStr, path::Path};

use crate::{
    ffi::{OpaqueUserData, cache::CSongCache},
    song_load::{
        LoadedSong, LoadedSongs, load_song_directory, load_song_directory_parallel,
        load_song_from_path,
    },
};

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

impl From<LoadedSong> for CLoadedSong {
    fn from(loaded_song: LoadedSong) -> Self {
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

impl From<LoadedSongs> for CLoadedSongs {
    fn from(loaded_songs: LoadedSongs) -> Self {
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

impl From<CLoadedSong> for LoadedSong {
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

        LoadedSong {
            hash: hash_str,
            path: std::path::PathBuf::from(path_str),
            song_length: duration,
        }
    }
}

impl From<CLoadedSongs> for LoadedSongs {
    fn from(c_loaded_songs: CLoadedSongs) -> Self {
        let songs_slice =
            unsafe { std::slice::from_raw_parts(c_loaded_songs.songs, c_loaded_songs.count) };

        let songs: Vec<LoadedSong> = songs_slice
            .iter()
            .map(|c_song| LoadedSong::from(*c_song))
            .collect();

        LoadedSongs { songs }
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
) -> CLoadedSong {
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

    let songs =
        load_song_directory(path, cache, wrapped.as_ref()).expect("Failed to load song directory");

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

    let songs = load_song_directory_parallel(&[path], cache, wrapped.as_ref())
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
        move |song: &LoadedSong, index, count| {
            let cloaded_song = CLoadedSong::from(song.clone());
            callback(cloaded_song, index, count, user_data);
            // from to avoid
            let _ = LoadedSong::from(cloaded_song);
        }
    });

    let songs = load_song_directory_parallel(&paths, cache, wrapped.as_ref())
        .expect("Failed to load song directory in parallel");

    let c_loaded_songs: CLoadedSongs = songs.into();
    c_loaded_songs
}

#[unsafe(no_mangle)]
pub extern "C" fn song_loader_free_loaded_song(loaded_song: CLoadedSong) {
    let _ = LoadedSong::from(loaded_song);
}

#[unsafe(no_mangle)]
pub extern "C" fn song_loader_free_loaded_songs(loaded_songs: CLoadedSongs) {
    let _ = LoadedSongs::from(loaded_songs);
}
