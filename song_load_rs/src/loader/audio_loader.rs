use std::time::Duration;

use bytes::Bytes;
use symphonia::{core::io::MediaSourceStream, default::get_probe};

/// Decodes the given audio bytes and returns their duration, if determinable.
#[tracing::instrument(level = "trace", skip(song))]
pub fn get_song_length_from(song: Bytes) -> Result<Option<Duration>, String> {
    let cursor = std::io::Cursor::new(song);
    let mss = MediaSourceStream::new(Box::new(cursor), Default::default());
    let probe = get_probe()
        .format(
            &Default::default(),
            mss,
            &Default::default(),
            &Default::default(),
        )
        .map_err(|s| format!("{s}"))?;
    let format = probe.format;

    if let Some(track) = format.default_track() {
        let sample_rate = track.codec_params.sample_rate.unwrap();
        let duration = track.codec_params.n_frames.unwrap() as f64 / sample_rate as f64;

        return Ok(Some(Duration::from_secs_f64(duration)));
    }

    Ok(None)
}

/// Reads a beatmap's song file (per its Info.dat) and returns its duration, if determinable.
#[cfg(all(feature = "beatmap", feature = "info-dat"))]
pub fn get_beatmap_song_length(
    beatmap: &crate::beatmap::BeatmapSource,
) -> Result<Option<Duration>, std::io::Error> {
    let (_, info_dat) = beatmap
        .get_info_dat()
        .map_err(|e| std::io::Error::other(format!("Failed to get Info.dat: {}", e)))?;

    let Some(song_filename) = &info_dat.get_song_filename() else {
        return Ok(None);
    };

    let song_bytes = beatmap.get_file_bytes(std::path::Path::new(song_filename))?;
    let length = get_song_length_from(song_bytes)
        .map_err(|e| std::io::Error::other(format!("Failed to get song length: {}", e)))?;
    Ok(length)
}
