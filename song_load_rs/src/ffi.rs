use std::{ffi::CStr, os::raw::c_void, path::Path};

use semver::Version;

use crate::{LoadedSong, cache::SongCache, song_load::{LoadedSongs, load_song_directory, load_song_directory_parallel, load_song_from_path}};

pub mod cache;
pub mod song_load;
pub mod version;


#[repr(C)]
#[derive(Clone, Copy)]
pub struct OpaqueUserData(*const c_void);

unsafe impl Sync for OpaqueUserData {}
unsafe impl Send for OpaqueUserData {}

#[unsafe(no_mangle)]
pub extern "C" fn hello_from_rust() {
    #[cfg(feature = "paper2-logging")]
    paper2_tracing::init_paper_tracing(Some("song_load_rs".to_owned()))
        .expect("Failed to initialize tracing");

    tracing::info!("Hello from Rust!");
}


