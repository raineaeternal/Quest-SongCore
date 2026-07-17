use std::{ffi::CStr, os::raw::c_char};

use crate::loader::audio_loader;

/// Reads the audio file at `path` and returns its duration in seconds,
/// or a negative value if the duration could not be determined.
/// # Safety
/// The `path` pointer must be a valid null-terminated C string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_get_audio_length_secs_from_path(path: *const c_char) -> f32 {
    if path.is_null() {
        return -1.0;
    }

    let Ok(path) = unsafe { CStr::from_ptr(path) }.to_str() else {
        return -1.0;
    };

    let Ok(bytes) = std::fs::read(path) else {
        return -1.0;
    };

    match audio_loader::get_song_length_from(bytes.into()) {
        Ok(Some(duration)) => duration.as_secs_f32(),
        _ => -1.0,
    }
}

/// Returns the duration in seconds of the audio data at `data` (of length
/// `len` bytes), or a negative value if the duration could not be determined.
/// # Safety
/// `data` must be valid for reads of `len` bytes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_get_audio_length_secs_from_bytes(
    data: *const u8,
    len: usize,
) -> f32 {
    if data.is_null() {
        return -1.0;
    }

    let bytes = unsafe { std::slice::from_raw_parts(data, len) };
    let bytes = bytes::Bytes::copy_from_slice(bytes);

    match audio_loader::get_song_length_from(bytes) {
        Ok(Some(duration)) => duration.as_secs_f32(),
        _ => -1.0,
    }
}
