use std::path::PathBuf;
use std::time::Duration;

// Verify that `get_song_length` returns a present, reasonable duration for
// both zip and directory beatmap sources. We avoid asserting exact seconds
// to keep the test robust across decoders.

#[test]
fn despacito_zip_length() -> Result<(), Box<dyn std::error::Error>> {
    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let zip_path = manifest_dir
        .join("tests")
        .join("f4c3 (Despacito - cookie).zip");

    let beatmap = song_load_rs::beatmap::Beatmap::from_path(&zip_path)?;
    let len = song_load_rs::audio_load::get_song_length(&beatmap)?;

    assert!(len.is_some(), "expected Some(duration) for zip");
    let d = len.unwrap();
    // Expected duration: 3 minutes 48 seconds == 228 seconds
    // Allow a small tolerance to account for decoder fractional differences.
    let expected = 228.0_f64;
    let tol = 2.0_f64; // seconds
    let measured = d.as_secs_f64();
    assert!(
        (measured - expected).abs() <= tol,
        "unexpected duration (secs): {} (allowed ±{}s)",
        measured,
        tol
    );

    Ok(())
}

#[test]
fn despacito_dir_length() -> Result<(), Box<dyn std::error::Error>> {
    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let dir = manifest_dir.join("tests").join("f4c3 (Despacito - cookie)");

    let beatmap = song_load_rs::beatmap::Beatmap::from_path(&dir)?;
    let len = song_load_rs::audio_load::get_song_length(&beatmap)?;

    assert!(len.is_some(), "expected Some(duration) for dir");
    let d = len.unwrap();
    // Expected duration: 3 minutes 48 seconds == 228 seconds
    // Allow a small tolerance to account for decoder fractional differences.
    let expected = 228.0_f64;
    let tol = 2.0_f64; // seconds
    let measured = d.as_secs_f64();
    assert!(
        (measured - expected).abs() <= tol,
        "unexpected duration (secs): {} (allowed ±{}s)",
        measured,
        tol
    );

    Ok(())
}
