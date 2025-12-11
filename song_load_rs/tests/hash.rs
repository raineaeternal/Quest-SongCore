use std::fs;
use std::path::PathBuf;

use song_load_rs::beatmap::Beatmap;

// TODO: download beatmap to avoid keeping it in the repo

#[test]
fn despacito_zip_hash() -> Result<(), Box<dyn std::error::Error>> {
    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let zip_path = manifest_dir
        .join("tests")
        .join("f4c3 (Despacito - cookie).zip");

    let zip_bytes = fs::read(&zip_path)?;
    let cursor = std::io::Cursor::new(bytes::Bytes::from(zip_bytes));
    let mut archive = zip::ZipArchive::new(cursor)?;

    let beatmap = Beatmap::Zip(archive.into());

    let hash = song_load_rs::hash::compute_custom_level_hash_from_beatmap(&beatmap)?;
    assert_eq!(hash, "4ed18607aee125f57cce73b45fc3934bbc899860");

    Ok(())
}

#[test]
fn despacito_dir_hash() -> Result<(), Box<dyn std::error::Error>> {
    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let dir = manifest_dir.join("tests").join("f4c3 (Despacito - cookie)");
    // Use the public API that computes hash from a directory path.

    let beatmap = Beatmap::from_path(&dir)?;
    let hash = song_load_rs::hash::compute_custom_level_hash_from_beatmap(&beatmap)?;
    assert_eq!(hash, "4ed18607aee125f57cce73b45fc3934bbc899860");

    Ok(())
}
