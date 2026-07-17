
// switch to use mimalloc as the global allocator
#[global_allocator]
static GLOBAL: mimalloc::MiMalloc = mimalloc::MiMalloc;

#[cfg(feature = "ffi")]
pub mod bindings;

pub mod utils;

#[cfg(feature = "ffi")]
pub mod ffi;
pub mod models;

/// Caching-related types and functions
pub mod cache;
/// Hashing-related types and functions
#[cfg(feature = "hashing")]
pub mod hash;
/// Loading various functions
pub mod loader;

/// Beatmap-related types and functions
#[cfg(feature = "beatmap")]
pub mod beatmap;
/// Info.dat type
#[cfg(feature = "info-dat")]
pub mod info_dat;
/// Version parsing
#[cfg(feature = "version-parsing")]
pub mod version;
