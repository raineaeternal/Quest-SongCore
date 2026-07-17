use std::{ffi::CString, os::raw::c_char};

#[cfg(all(feature = "info-dat", feature = "beatmap"))]
use std::{ffi::CStr, path::PathBuf};

#[cfg(all(feature = "info-dat", feature = "beatmap"))]
use crate::{beatmap::BeatmapSource, hash::compute_custom_level_hash_from_beatmap};

/// Computes the SHA-1 hash of the beatmap (zip file or directory) at `path`.
/// Returns a heap-allocated, null-terminated C string on success, or null if
/// the hash could not be computed. Caller must free the returned pointer with
/// `song_core_free_string`.
/// # Safety
/// The `path` pointer must be a valid null-terminated C string.
#[cfg(all(feature = "info-dat", feature = "beatmap"))]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_get_beatmap_hash_from_path(path: *const c_char) -> *mut c_char {
    if path.is_null() {
        return std::ptr::null_mut();
    }

    let Ok(path) = unsafe { CStr::from_ptr(path) }.to_str()  else {
        return std::ptr::null_mut();
    };

    let Ok(beatmap) = BeatmapSource::from_path(PathBuf::from(path)) else {
        return std::ptr::null_mut();
    };

    let Ok(hash) = compute_custom_level_hash_from_beatmap(&beatmap) else {
        return std::ptr::null_mut();
    };

    let Ok(c_hash) = CString::new(hash) else {
        return std::ptr::null_mut();
    };

    c_hash.into_raw()
}

/// Computes the SHA-1 hash of the beatmap (zip file or directory) at `path`
/// and writes it as a null-terminated hex string into `out`, avoiding a heap
/// allocation. `out_len` must be at least 41 (40 hex characters plus the
/// null terminator; SHA-1 digests are a fixed size). Returns `true` on
/// success, `false` if the hash could not be computed or `out` was too small.
/// # Safety
/// `path` must be a valid null-terminated C string. `out` must be valid for
/// writes of `out_len` bytes.
#[cfg(all(feature = "info-dat", feature = "beatmap"))]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_get_beatmap_hash_from_path_zerocopy(
    path: *const c_char,
    out: *mut c_char,
    out_len: usize,
) -> bool {
    if path.is_null() || out.is_null() {
        return false;
    }

    let Ok(path) = unsafe { CStr::from_ptr(path) }.to_str()  else {
        return false;
    };

    let Ok(beatmap) = BeatmapSource::from_path(PathBuf::from(path)) else {
        return false;
    };

    let Ok(hash) = compute_custom_level_hash_from_beatmap(&beatmap) else {
        return false;
    };

    if hash.len() + 1 > out_len {
        return false;
    }

    let out_slice = unsafe { std::slice::from_raw_parts_mut(out as *mut u8, out_len) };
    out_slice[..hash.len()].copy_from_slice(hash.as_bytes());
    out_slice[hash.len()] = 0;

    true
}
