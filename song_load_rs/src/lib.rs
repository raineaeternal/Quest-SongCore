use std::{ffi::CStr, path::Path};

use crate::{cache::SongCache, song_load::LoadedSong};

pub mod bindings;

pub mod cache;
pub mod hash;
pub mod models;
pub mod song_load;

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

        song_load::LoadedSong {
            hash: hash_str,
            path: std::path::PathBuf::from(path_str),
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
) -> CLoadedSongs {
    if path.is_null() {
        panic!("Path is null");
    }
    let path = unsafe { CStr::from_ptr(path) }
        .to_str()
        .map(Path::new)
        .expect("Failed to convert path to str");

    let cache = unsafe { cache.as_mut().map(|c| c.inner.as_mut()) };

    let songs = song_load::load_song_directory(path, cache).expect("Failed to load song directory");

    let c_loaded_songs: CLoadedSongs = songs.into();
    c_loaded_songs
}

#[unsafe(no_mangle)]
pub extern "C" fn song_loader_free_loaded_song(loaded_song: CLoadedSong) {
    let _ = LoadedSong::from(loaded_song);
}

#[unsafe(no_mangle)]
pub extern "C" fn song_loader_free_loaded_songs(loaded_songs: CLoadedSongs) {
    let _ = song_load::LoadedSongs::from(loaded_songs);
}
