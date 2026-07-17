use std::ffi::{CString, c_char};

#[cfg(feature = "audio-loading")]
pub mod audio;
#[cfg(feature = "metadata-loading")]
pub mod beatmap_metadata_load;
pub mod cache;
#[cfg(feature = "hashing")]
pub mod hash;
#[cfg(feature = "version-parsing")]
pub mod version;

#[cfg(feature = "level-loading")]
pub mod level_loader;

pub mod types;

/// Initializes Rust components, such as logging/tracing.
#[unsafe(no_mangle)]
pub extern "C" fn songcore_init_rust() {
    #[cfg(all(feature = "paper2-logging", target_os = "android"))]
    {
        // Use paper2_tracing's PaperLayer but allow configuring the tracing
        // filter via `RUST_LOG` (if set) or a reasonable default that
        // silences Symphonia debug logs.
        use tracing_subscriber::{registry::Registry, EnvFilter};
        use tracing_subscriber::layer::SubscriberExt;
        use tracing_subscriber::util::SubscriberInitExt;

        let default_filter = "info,symphonia=warn,symphonia_core=warn,symphonia_format_ogg=warn";
        let filter_str = std::env::var("RUST_LOG").unwrap_or_else(|_| default_filter.to_string());
        let env_filter = EnvFilter::try_new(filter_str).unwrap_or_else(|_| EnvFilter::new(default_filter));

        let layer = paper2_tracing::PaperLayer::new().with_tag("song_load_rs");

        Registry::default().with(env_filter).with(layer).try_init()
            .expect("Failed to initialize tracing subscriber");
    }

    tracing::info!("Hello from Rust!");
}

/// Frees a string returned by a `song_load_rs` function that documents
/// freeing via this function (e.g. `song_core_get_beatmap_hash_from_path`).
/// # Safety
/// The pointer must not be used again after this call.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_free_string(ptr: *mut c_char) {
    if ptr.is_null() {
        return;
    }
    unsafe {
        let _ = CString::from_raw(ptr);
    }
}
