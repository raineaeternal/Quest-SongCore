use std::path::PathBuf;

// Smoke tests for the public `song_load` APIs that exercise loading from
// a zip file and a directory under the repository `tests/` folder.

#[test]
fn load_song_from_zip_and_directory_match() -> Result<(), Box<dyn std::error::Error>> {
    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let tests_dir = manifest_dir.join("tests");

    // Known test resource in this repo
    let zip_path = tests_dir.join("f4c3 (Despacito - cookie).zip");
    let dir_path = tests_dir.join("f4c3 (Despacito - cookie)");

    // Load from zip
    let song_from_zip = song_load_rs::song_load::load_song_from_path(zip_path.clone())
        .map_err(|e| format!("load from zip failed: {}", e))?;

    // Load from directory (the public API accepts both file and directory)
    let song_from_dir = song_load_rs::song_load::load_song_from_path(dir_path.clone())
        .map_err(|e| format!("load from dir failed: {}", e))?;

    // The hashes should match
    assert_eq!(song_from_zip.hash, song_from_dir.hash);

    // The expected hash for the included test resource
    let expected = "4ed18607aee125f57cce73b45fc3934bbc899860";
    assert_eq!(song_from_zip.hash, expected);

    Ok(())
}

#[test]
fn load_song_directory_finds_songs() -> Result<(), Box<dyn std::error::Error>> {
    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let tests_dir = manifest_dir.join("tests");

    // Use the directory-loading API which scans entries in the given folder
    let loaded = song_load_rs::song_load::load_song_directory(&tests_dir)
        .map_err(|e| format!("load_song_directory failed: {}", e))?;

    // Expect at least one song was found
    assert!(
        !loaded.songs.is_empty(),
        "no songs found in tests directory"
    );

    // Ensure our known song is present with the expected hash
    let expected = "4ed18607aee125f57cce73b45fc3934bbc899860";
    let found = loaded.songs.iter().all(|s| s.hash == expected);
    assert!(found, "expected test song hash not found in loaded songs");

    Ok(())
}
