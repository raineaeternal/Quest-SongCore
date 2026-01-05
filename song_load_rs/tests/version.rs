use std::path::PathBuf;

use song_load_rs::version::{get_version, version_from_file_path};

#[test]
fn parses_version_from_bytes() {
    let manifest_dir = env!("CARGO_MANIFEST_DIR");
    let path = PathBuf::from(manifest_dir)
        .join("tests")
        .join("f4c3 (Despacito - cookie)")
        .join("Info.dat");

    let data = std::fs::read(&path).expect("failed to read Info.dat");
    let ver = get_version(&data).expect("failed to parse version from bytes");

    assert_eq!(ver.major, 2);
    assert_eq!(ver.minor, 0);
    assert_eq!(ver.patch, 0);
}

#[test]
fn parses_version_from_file_path() {
    let manifest_dir = env!("CARGO_MANIFEST_DIR");
    let path = PathBuf::from(manifest_dir)
        .join("tests")
        .join("f4c3 (Despacito - cookie)")
        .join("Info.dat");

    let ver = version_from_file_path(&path).expect("failed to parse version from file");

    assert_eq!(ver.major, 2);
    assert_eq!(ver.minor, 0);
    assert_eq!(ver.patch, 0);
}
