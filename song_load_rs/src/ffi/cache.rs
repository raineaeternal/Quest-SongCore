use std::{ffi::CStr, path::Path};

use crate::ffi::song_load::CSongCache;


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