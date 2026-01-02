pub mod beatmap_metadata_load;
pub mod cache;
pub mod version;
pub mod level_loader;

pub mod types;

#[unsafe(no_mangle)]
pub extern "C" fn hello_from_rust() {
    #[cfg(feature = "paper2-logging")]
    paper2_tracing::init_paper_tracing(Some("song_load_rs".to_owned()))
        .expect("Failed to initialize tracing");

    tracing::info!("Hello from Rust!");
}
