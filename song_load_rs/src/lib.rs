
// switch to use mimalloc as the global allocator
#[global_allocator]
static GLOBAL: mimalloc::MiMalloc = mimalloc::MiMalloc;

pub mod bindings;

pub mod utils;

pub mod ffi;
pub mod models;

/// Caching-related types and functions
pub mod cache;
/// Hashing-related types and functions
pub mod hash;
/// Loading various functions
pub mod loader;

/// Beatmap-related types and functions
pub mod beatmap;
/// Info.dat type
pub mod info_dat;
/// Version parsing
pub mod version;
