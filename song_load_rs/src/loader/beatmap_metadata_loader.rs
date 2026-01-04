use std::{
    fs::DirEntry,
    io,
    path::{Path, PathBuf},
    sync::{RwLock, atomic::AtomicUsize},
    time::Duration,
};

use rayon::iter::{ParallelBridge, ParallelIterator};
use serde::{Deserialize, Serialize};
use thiserror::Error;
use tracing::{info, warn};

use crate::{
    beatmap::BeatmapSource,
    cache::{CacheError, SongCache},
    hash::compute_custom_level_hash_from_beatmap,
    loader::audio_loader,
};

pub const CUSTOM_LEVEL_PREFIX_ID: &str = "custom_level_";

/// A struct representing useful cached data for a loaded song.
#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Serialize, Deserialize)]
pub struct BeatmapMetadata {
    /// Path to the beatmap
    pub path: PathBuf,
    /// SHA1 hash of the beatmap
    pub hash: String,
    /// Optional length of the song if known
    pub song_length: Option<Duration>,
}

#[derive(Clone)]
pub struct BeatmapMetadataArray {
    pub songs: Vec<BeatmapMetadata>,
}

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
    cache: Option<&RwLock<C>>,
) -> Result<BeatmapMetadata, LoadBeatmapMetadataError>
where
    C: SongCache + ?Sized,
{
    let path = beatmap.get_real_path().to_path_buf();
    info!("Loading beatmap metadata from path: {:?}", path);

    let cached = cache
        .map(|c| c.read().unwrap().get_cached_song(&path))
        .transpose()?
        .flatten();

    // Return cached hash if available
    if let Some(cached) = cached {
        return Ok(cached);
    }

    let hash = compute_custom_level_hash_from_beatmap(beatmap)?;
    let song_length = audio_loader::get_song_length(beatmap).map_err(|e| {
        LoadBeatmapMetadataError::ComputeHashError(format!("Failed to get song length: {}", e))
    })?;

    if let Some(c) = &cache {
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
    cache: Option<&RwLock<C>>,
) -> Result<BeatmapMetadata, LoadBeatmapMetadataError>
where
    C: SongCache + ?Sized,
{
    if !path.exists() {
        return Err(LoadBeatmapMetadataError::PathDoesNotExist(path));
    }
    let beatmap = BeatmapSource::from_path(path)?;

    load_beatmap_metadata(&beatmap, cache)
}

/// Loads all songs from the given directory.
/// Returns an error if the path does not exist or is not a directory.
///
/// Synchronous version.
pub fn load_beatmap_directory<C, F>(
    path: &std::path::Path,
    cache: Option<&RwLock<C>>,
    callback: Option<F>,
) -> Result<BeatmapMetadataArray, LoadBeatmapMetadataError>
where
    F: Fn(&BeatmapMetadata, usize, usize),
    C: SongCache + ?Sized,
{
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
            let song_data = match load_beatmap_from_path::<C>(entry.path(), cache) {
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

    Ok(BeatmapMetadataArray {
        songs: loaded_songs,
    })
}

// TODO: Add progress reporting
/// Loads all songs from the given directory in parallel.
/// Returns an error if the path does not exist or is not a directory.
/// Parallel version.
pub fn load_beatmap_directory_parallel<F, C>(
    paths: &[&Path],
    cache: Option<&RwLock<C>>,
    callback: Option<F>,
) -> Result<BeatmapMetadataArray, LoadBeatmapMetadataError>
where
    F: Fn(&BeatmapMetadata, usize, usize) + Sync,
    C: SongCache + ?Sized,
{
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
        .collect::<Result<Vec<_>, _>>()?;
    let read_dir_entries: Vec<DirEntry> = read_dir_entries
        .into_iter()
        .flat_map(|r| r.filter_map(|e| e.ok()))
        .collect();

    let count = read_dir_entries.len();

    let worked = AtomicUsize::new(0);

    let loaded_songs: Vec<BeatmapMetadata> = read_dir_entries
        .into_iter()
        .par_bridge()
        .filter_map(|entry| {
            let path = entry.path();
            // we cache later in bulk
            let song_data = match load_beatmap_from_path::<C>(entry.path(), cache) {
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

    Ok(BeatmapMetadataArray {
        songs: loaded_songs,
    })
}
