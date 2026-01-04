pub mod beatmap_metadata_load;
pub mod cache;
pub mod version;

#[cfg(feature = "level-loading")]
pub mod level_loader;

pub mod types;

/// Initializes Rust components, such as logging/tracing.
#[unsafe(no_mangle)]
pub extern "C" fn songcore_init_rust() {
    #[cfg(all(feature = "paper2-logging", target_os = "android"))]
    paper2_tracing::init_paper_tracing(Some("song_load_rs".to_owned()))
        .expect("Failed to initialize tracing");

    tracing::info!("Hello from Rust!");
}
