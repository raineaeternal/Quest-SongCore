use std::{io, path::PathBuf};

#[cfg(not(feature = "audio-loading"))]
use std::time::Duration;

#[cfg(feature = "parallel")]
use std::{path::Path, sync::atomic::AtomicUsize};

#[cfg(feature = "parallel")]
use rayon::iter::{IntoParallelIterator, ParallelIterator};
use thiserror::Error;
use tracing::{info, info_span, warn};

#[cfg(feature = "audio-loading")]
use crate::loader::audio_loader;
pub use crate::models::metadata::{BeatmapMetadata, BeatmapMetadataArray};
use crate::{
    beatmap::BeatmapSource,
    cache::{CacheError, SongCache},
    hash::compute_custom_level_hash_from_beatmap,
    utils::SongCoreLock,
};

pub const CUSTOM_LEVEL_PREFIX_ID: &str = "custom_level_";

#[derive(Debug, Error)]
pub enum LoadBeatmapMetadataError {
    #[error("Path does not exist")]
    PathDoesNotExist(PathBuf),

    #[error("Failed to compute hash: {0}")]
    ComputeHashError(String),

    #[error("Audio processing error: {0}")]
    AudioError(io::Error),

    #[error("Cache error: {0}")]
    CacheError(#[from] CacheError),

    #[error("I/O error: {0}")]
    IoError(#[from] std::io::Error),
}

/// Loads a song from the given BeatmapSource, optionally using the provided cache.
pub fn load_beatmap_metadata<C>(
    beatmap: &BeatmapSource,
    cache: Option<&SongCoreLock<C>>,
    save: bool,
) -> Result<BeatmapMetadata, LoadBeatmapMetadataError>
where
    C: SongCache + ?Sized,
{
    let path = beatmap.get_real_path().to_path_buf();
    let span = info_span!("load_beatmap_metadata", path = %path.display());
    let _enter = span.enter();

    let cached = cache
        .map(|c| c.read().unwrap().get_cached_song(&path))
        .transpose()?
        .flatten();

    // Return cached hash if available
    if let Some(cached) = cached {
        return Ok(cached);
    }

    // Measure hash and audio timing separately for profiling.
    let hash_start = std::time::Instant::now();
    let hash = compute_custom_level_hash_from_beatmap(beatmap)?;
    let hash_ms = hash_start.elapsed().as_millis();

    let audio_start = std::time::Instant::now();
    #[cfg(feature = "audio-loading")]
    let song_length = audio_loader::get_beatmap_song_length(beatmap).map_err(|e| {
        LoadBeatmapMetadataError::ComputeHashError(format!("Failed to get song length: {}", e))
    })?;
    #[cfg(not(feature = "audio-loading"))]
    let song_length: Option<Duration> = None;
    let audio_ms = audio_start.elapsed().as_millis();

    info!(
        hash_ms = hash_ms,
        audio_ms = audio_ms,
        "Loaded beatmap metadata"
    );

    if save && let Some(c) = &cache {
        c.write().unwrap().cache_song(BeatmapMetadata {
            hash: hash.clone(),
            path: path.clone(),
            song_length,
        })?;
    }

    Ok(BeatmapMetadata {
        hash,
        path,
        song_length,
    })
}

/// Loads a song from the given file path, optionally using the provided cache.
pub fn load_beatmap_from_path<C>(
    path: PathBuf,
    cache: Option<&SongCoreLock<C>>,
    save: bool,
) -> Result<BeatmapMetadata, LoadBeatmapMetadataError>
where
    C: SongCache + ?Sized,
{
    if !path.exists() {
        return Err(LoadBeatmapMetadataError::PathDoesNotExist(path));
    }
    let span = info_span!("load_beatmap_from_path", path = %path.display());
    let _enter = span.enter();

    let beatmap = BeatmapSource::from_path(path)?;

    load_beatmap_metadata(&beatmap, cache, save)
}

/// Loads all songs from the given directory.
/// Returns an error if the path does not exist or is not a directory.
///
/// Synchronous version.
pub fn load_beatmap_directory<C, F>(
    path: &std::path::Path,
    cache: Option<&SongCoreLock<C>>,
    callback: Option<F>,
) -> Result<BeatmapMetadataArray, LoadBeatmapMetadataError>
where
    F: Fn(&BeatmapMetadata, usize, usize),
    C: SongCache + ?Sized,
{
    let span = info_span!("load_beatmap_directory", path = %path.display());
    let _enter = span.enter();
    info!("Loading beatmap directory");

    if !path.exists() || !path.is_dir() {
        return Err(LoadBeatmapMetadataError::PathDoesNotExist(
            path.to_path_buf(),
        ));
    }

    // read_dir is fine here, we don't need recursion
    let read_dir_entries: Vec<_> = std::fs::read_dir(path)?.collect();
    let count = read_dir_entries.len();

    let loaded_songs: Vec<BeatmapMetadata> = read_dir_entries
        .into_iter()
        .enumerate()
        .filter_map(|(i, entry)| {
            let entry = entry.ok()?;
            let path = entry.path();
            // we cache later in bulk
            let song_data = match load_beatmap_from_path::<C>(entry.path(), cache, false) {
                Ok(data) => data,
                Err(e) => {
                    warn!("Failed to load song from path {:?}: {}", path, e);
                    return None;
                }
            };

            if let Some(cb) = &callback {
                cb(&song_data, i, count);
            }

            Some(song_data)
        })
        .collect();

    // cache in bulk
    if let Some(c) = &cache {
        let mut write = c.write().unwrap();
        for song in &loaded_songs {
            write.cache_song(song.clone())?;
        }
    }

    Ok(BeatmapMetadataArray {
        songs: loaded_songs,
    })
}

// TODO: Add progress reporting
/// Loads all songs from the given directory in parallel.
/// Returns an error if the path does not exist or is not a directory.
/// Parallel version.
#[cfg(feature = "parallel")]
pub fn load_beatmap_directory_parallel<F, C>(
    paths: &[&Path],
    cache: Option<&SongCoreLock<C>>,
    callback: Option<F>,
) -> Result<BeatmapMetadataArray, LoadBeatmapMetadataError>
where
    F: Fn(&BeatmapMetadata, usize, usize) + Sync,
    C: SongCache + ?Sized,
{
    let span = info_span!("load_beatmap_directory_parallel", dirs = ?paths);
    let _enter = span.enter();
    info!("Loading beatmap directories in parallel");

    let start = std::time::Instant::now();
    // validate paths
    for path in paths {
        if !path.exists() || !path.is_dir() {
            return Err(LoadBeatmapMetadataError::PathDoesNotExist(
                path.to_path_buf(),
            ));
        }
    }

    // Compute hashes in parallel but do not touch the mutable cache from the parallel closure.
    let read_dir_entries: Vec<_> = paths
        .iter()
        .map(std::fs::read_dir)
        .collect::<Result<_, _>>()?;

    let paths: Vec<PathBuf> = read_dir_entries
        .into_iter()
        .flat_map(|dir_iter| dir_iter.filter_map(Result::ok))
        .map(|entry| entry.path())
        .collect();

    info!(
        "Found {} beatmap paths to load in time {}ms",
        paths.len(),
        (start.elapsed().as_millis())
    );

    let count = paths.len();

    let worked = AtomicUsize::new(0);
    let loaded_songs: Vec<BeatmapMetadata> = paths
        .into_par_iter()
        .filter_map(|path| {
            // we cache later in bulk
            let song_data = match load_beatmap_from_path::<C>(path.clone(), cache, false) {
                Ok(data) => data,
                Err(e) => {
                    warn!("Failed to load song from path {:?}: {}", path, e);
                    return None;
                }
            };

            if let Some(cb) = &callback {
                let worked = worked.fetch_add(1, std::sync::atomic::Ordering::AcqRel);
                cb(&song_data, worked, count);
            }

            Some(song_data)
        })
        .collect();
    info!(
        "Finished processing beatmaps in parallel in {}ms",
        start.elapsed().as_millis()
    );

    // cache in bulk
    if let Some(c) = &cache {
        let mut write = c.write().unwrap();
        write.cache_songs(loaded_songs.clone())?;
    }
    info!(
        "Cached loaded beatmaps in {}ms",
        start.elapsed().as_millis()
    );

    let end = std::time::Instant::now();
    info!(
        "Loaded {} beatmaps in {}ms (parallel)",
        loaded_songs.len(),
        (end - start).as_millis()
    );
    Ok(BeatmapMetadataArray {
        songs: loaded_songs,
    })
}
