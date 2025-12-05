use tracing::info;

pub mod bindings;

#[unsafe(no_mangle)]
pub extern "C" fn hello_from_rust() {
    paper2_tracing::init_paper_tracing(Some("song_load_rs".to_owned())).expect("Failed to initialize tracing");

    info!("Hello from Rust!");
}
