use std::{ffi::CStr, path::Path};

use crate::{
    ffi::{cache::CSongCache, types::OpaqueUserData},
    loader::beatmap_metadata_loader::{
        BeatmapMetadata, BeatmapMetadataArray, load_beatmap_directory, load_beatmap_directory_parallel,
        load_beatmap_from_path,
    },
};

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CBeatmapMetadata {
    pub path: *const std::os::raw::c_char,
    pub hash: *const std::os::raw::c_char,
    pub duration_secs: u64,
    pub duration_nanos: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CBeatmapMetadataArray {
    pub songs: *const CBeatmapMetadata,
    pub count: usize,
}

impl From<BeatmapMetadata> for CBeatmapMetadata {
    fn from(loaded_song: BeatmapMetadata) -> Self {
        let c_path = std::ffi::CString::new(loaded_song.path.to_str().unwrap()).unwrap();
        let c_hash = std::ffi::CString::new(loaded_song.hash).unwrap();

        CBeatmapMetadata {
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

impl From<BeatmapMetadataArray> for CBeatmapMetadataArray {
    fn from(loaded_songs: BeatmapMetadataArray) -> Self {
        let c_songs: Vec<CBeatmapMetadata> = loaded_songs
            .songs
            .into_iter()
            .map(CBeatmapMetadata::from)
            .collect();

        let slice = c_songs.into_boxed_slice();
        let songs_ptr = slice.as_ptr();
        let count = slice.len();

        CBeatmapMetadataArray {
            songs: songs_ptr,
            count,
        }
    }
}

impl From<CBeatmapMetadata> for BeatmapMetadata {
    fn from(c_loaded_song: CBeatmapMetadata) -> Self {
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

        BeatmapMetadata {
            hash: hash_str,
            path: std::path::PathBuf::from(path_str),
            song_length: duration,
        }
    }
}

impl From<CBeatmapMetadataArray> for BeatmapMetadataArray {
    fn from(c_loaded_songs: CBeatmapMetadataArray) -> Self {
        let songs_slice = unsafe {
            std::slice::from_raw_parts_mut(
                c_loaded_songs.songs as *mut CBeatmapMetadata,
                c_loaded_songs.count,
            )
        };

        let songs = unsafe { Box::from_raw(songs_slice) };

        let songs: Vec<BeatmapMetadata> = songs
            .into_vec()
            .iter()
            .map(|c_song| BeatmapMetadata::from(*c_song))
            .collect();

        BeatmapMetadataArray { songs }
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
pub unsafe extern "C" fn song_core_load_path(
    path: *const std::os::raw::c_char,
    cache: *mut CSongCache,
) -> CBeatmapMetadata {
    if path.is_null() {
        panic!("Path is null");
    }
    let path = unsafe { CStr::from_ptr(path) }
        .to_str()
        .map(Path::new)
        .expect("Failed to convert path to str");

    let cache = unsafe { cache.as_ref().map(|c| c.inner.as_ref()) };
    

    let song_load =
        load_beatmap_from_path(path.into(), cache).expect("Failed to load song from path");

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
pub unsafe extern "C" fn song_core_load_directory(
    path: *const std::os::raw::c_char,
    cache: *mut CSongCache,
    user_data: OpaqueUserData,
    fn_callback: Option<extern "C" fn(CBeatmapMetadata, usize, usize, OpaqueUserData)>,
) -> CBeatmapMetadataArray {
    if path.is_null() {
        panic!("Path is null");
    }
    let path = unsafe { CStr::from_ptr(path) }
        .to_str()
        .map(Path::new)
        .expect("Failed to convert path to str");

    let cache = unsafe { cache.as_ref().map(|c| c.inner.as_ref()) };
    let wrapped = fn_callback.map(|callback| {
        move |song: &BeatmapMetadata, index, count| {
            let cloaded_song = CBeatmapMetadata::from(song.clone());
            callback(cloaded_song, index, count, user_data);
            // from to avoid
            let _ = BeatmapMetadata::from(cloaded_song);
        }
    });

    let songs = load_beatmap_directory(path, cache, wrapped.as_ref())
        .expect("Failed to load song directory");

    let c_loaded_songs: CBeatmapMetadataArray = songs.into();
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
pub unsafe extern "C" fn song_core_load_directory_parallel(
    path: *const std::os::raw::c_char,
    cache: *mut CSongCache,
    user_data: OpaqueUserData,
    fn_callback: Option<extern "C" fn(CBeatmapMetadata, usize, usize, OpaqueUserData)>,
) -> CBeatmapMetadataArray {
    if path.is_null() {
        panic!("Path is null");
    }
    let path = unsafe { CStr::from_ptr(path) }
        .to_str()
        .map(Path::new)
        .expect("Failed to convert path to str");

    let cache = unsafe { cache.as_ref().map(|c| c.inner.as_ref()) };
    let wrapped = fn_callback.map(|callback| {
        move |song: &BeatmapMetadata, index, count| {
            let cloaded_song = CBeatmapMetadata::from(song.clone());
            callback(cloaded_song, index, count, user_data);
            // from to avoid
            let _ = BeatmapMetadata::from(cloaded_song);
        }
    });

    let songs = load_beatmap_directory_parallel(&[path], cache, wrapped.as_ref())
        .expect("Failed to load song directory in parallel");

    let c_loaded_songs: CBeatmapMetadataArray = songs.into();
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
pub unsafe extern "C" fn song_core_load_directories_parallel(
    paths: *const *const std::os::raw::c_char,
    path_count: usize,
    cache: *mut CSongCache,
    user_data: OpaqueUserData,
    fn_callback: Option<extern "C" fn(CBeatmapMetadata, usize, usize, OpaqueUserData)>,
) -> CBeatmapMetadataArray {
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

    let cache = unsafe { cache.as_ref().map(|c| c.inner.as_ref()) };
    let wrapped = fn_callback.map(|callback| {
        move |song: &BeatmapMetadata, index, count| {
            let cloaded_song = CBeatmapMetadata::from(song.clone());
            callback(cloaded_song, index, count, user_data);
            // from to avoid
            let _ = BeatmapMetadata::from(cloaded_song);
        }
    });

    let songs = load_beatmap_directory_parallel(&paths, cache, wrapped.as_ref())
        .expect("Failed to load song directory in parallel");

    let c_loaded_songs: CBeatmapMetadataArray = songs.into();
    c_loaded_songs
}

#[unsafe(no_mangle)]
pub extern "C" fn song_core_free_loaded_song(loaded_song: CBeatmapMetadata) {
    let _ = BeatmapMetadata::from(loaded_song);
}

#[unsafe(no_mangle)]
pub extern "C" fn song_core_free_loaded_songs(loaded_songs: CBeatmapMetadataArray) {
    let _ = BeatmapMetadataArray::from(loaded_songs);
}
