use std::{path::Path, time::Duration, io};

use bytes::Bytes;
use symphonia::{core::io::MediaSourceStream, default::get_probe};

use crate::beatmap;

#[tracing::instrument(level = "trace", skip(song))]
fn get_song_length_from(song: Bytes) -> Result<Option<Duration>, String> {
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

/// Get the song length from a beatmap source (zip or directory).
pub fn get_song_length(
    beatmap: &beatmap::BeatmapSource,
) -> Result<Option<Duration>, io::Error> {
    let (_, info_dat) = beatmap
        .get_info_dat()
        .map_err(|e| io::Error::other(format!("Failed to get Info.dat: {}", e)))?;

    let Some(song_filename) = &info_dat.get_song_filename() else {
        return Ok(None);
    };

    let song_bytes = beatmap.get_file_bytes(Path::new(song_filename))?;
    let length = get_song_length_from(song_bytes)
        .map_err(|e| io::Error::other(format!("Failed to get song length: {}", e)))?;
    Ok(length)
}

/// Asynchronous version of `get_song_length`.
pub async fn get_song_length_async(
    beatmap: &beatmap::BeatmapSource,
) -> Result<Option<Duration>, io::Error> {
    let (_, info_dat) = beatmap
        .get_info_dat_async()
        .await
        .map_err(|e| io::Error::other(format!("Failed to get Info.dat: {}", e)))?;

    let Some(song_filename) = &info_dat.get_song_filename() else {
        return Ok(None);
    };

    let song_bytes = beatmap
        .get_file_bytes_async(Path::new(song_filename))
        .await?;
    let length = tokio::task::spawn_blocking(move || {
        get_song_length_from(song_bytes)
            .map_err(|e| io::Error::other(format!("Failed to get song length: {}", e)))
    })
    .await
    .map_err(|e| io::Error::other(format!("Task join error: {}", e)))??;

    Ok(length)
}