use std::{ffi::CStr, path::Path, sync::RwLock};

use crate::cache::SongCache;

/// Represents a song cache trait for use in FFI.
/// 
/// Unlike the Rust `SongCache` trait, this struct is guaranteed to be thread safe
/// by wrapping the inner cache in an `RwLock`.
#[repr(C)]
pub struct CSongCache {
    pub inner: Box<RwLock<dyn SongCache>>,
}

/// Creates a new file based song cache and returns a pointer to it.
/// # Safety
/// The caller is responsible for freeing the returned pointer using `song_core_free_song_cache`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_file_cache_new(
    path: *const std::os::raw::c_char,
) -> *mut CSongCache {
    if path.is_null() {
        panic!("Path is null");
    }
    let path = unsafe { CStr::from_ptr(path) }
        .to_str()
        .map(Path::new)
        .expect("Failed to convert path to str");

    let file_cache = crate::cache::file_cache::FileCache::new(path.into());

    let song_cache = CSongCache {
        inner: Box::new(RwLock::new(file_cache)),
    };

    Box::into_raw(Box::new(song_cache))
}

/// Reloads the cache from the source.
/// # Safety
/// The `cache` pointer must be a valid pointer to a `CSongCache`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_cache_load(cache: *mut CSongCache) {
    if cache.is_null() {
        panic!("Cache is null");
    }
    let cache = unsafe { cache.as_mut().unwrap() };

    cache
        .inner
        .write()
        .unwrap()
        .reload_cache()
        .expect("Failed to reload cache");
}

/// Saves the cache to the source.
/// # Safety
/// The `cache` pointer must be a valid pointer to a `CSongCache`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_cache_save(cache: *const CSongCache) {
    if cache.is_null() {
        panic!("Cache is null");
    }
    let cache = unsafe { cache.as_ref().unwrap() };

    cache
        .inner
        .read()
        .unwrap()
        .save_cache()
        .expect("Failed to reload cache");
}

/// Resets the cached data for the given song path.
/// # Safety
/// The `cache` pointer must be a valid pointer to a `CSongCache`.
/// The `song_path` pointer must be a valid null-terminated C string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_cache_reset_song(
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
        .write()
        .unwrap()
        .reset_song_cache(song_path)
        .expect("Failed to reset song cache");
}

/// Clears the entire song cache.
/// # Safety
/// The `cache` pointer must be a valid pointer to a `CSongCache`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_cache_clear(cache: *mut CSongCache) {
    if cache.is_null() {
        panic!("Cache is null");
    }
    let cache = unsafe { cache.as_mut().unwrap() };

    cache.inner.write().unwrap().clear_cache().expect("Failed to clear cache");
}

/// Checks if the cache contains a cached song for the given path.
/// # Safety
/// The `cache` pointer must be a valid pointer to a `CSongCache`.
/// The `song_path` pointer must be a valid null-terminated C string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_cache_contains_cache(
    cache: *const CSongCache,
    song_path: *const std::os::raw::c_char,
) -> bool {
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

    match cache.inner.read().unwrap().get_cached_song(song_path) {
        Ok(opt) => opt.is_some(),
        Err(e) => {
            panic!("Failed to check if cache contains song: {}", e);
        }
    }
}

/// Frees the given song cache.
/// # Safety
/// The `cache` pointer must be a valid pointer to a `CSongCache`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_free_song_cache(cache: *mut CSongCache) {
    if cache.is_null() {
        return;
    }
    unsafe {
        let _ = Box::from_raw(cache);
    }
}
