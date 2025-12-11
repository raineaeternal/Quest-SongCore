use std::path::{Path, PathBuf};

use rayon::iter::{ParallelBridge, ParallelIterator};
use thiserror::Error;
use tracing::warn;

use crate::{
    cache::{CacheError, SongCache},
    hash::compute_custom_level_from_path,
};

#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct LoadedSong {
    pub path: PathBuf,
    pub hash: String,
}

#[derive(Clone)]
pub struct LoadedSongs {
    pub songs: Vec<LoadedSong>,
}

#[derive(Debug, Error)]
pub enum LoadSongError {
    #[error("Path does not exist")]
    PathDoesNotExist(PathBuf),
    #[error("Failed to compute hash: {0}")]
    ComputeHashError(String),
    #[error("Cache error: {0}")]
    CacheError(#[from] CacheError),

    #[error("I/O error: {0}")]
    IoError(#[from] std::io::Error),
}

pub fn load_song_from_path(
    path: PathBuf,
    mut cache: Option<&mut dyn SongCache>,
) -> Result<LoadedSong, LoadSongError> {
    if !path.exists() {
        return Err(LoadSongError::PathDoesNotExist(path));
    }

    let cached_hash = cache
        .as_mut()
        .map(|c| c.get_cached_song(&path))
        .transpose()?
        .flatten();

    // Return cached hash if available
    if let Some(cached) = cached_hash {
        return Ok(cached);
    }

    let hash = compute_custom_level_from_path(&path.to_path_buf())?;

    if let Some(c) = cache {
        c.cache_song(LoadedSong {
            hash: hash.clone(),
            path: path.clone(),
        })?;
    }

    Ok(LoadedSong { hash, path })
}

/// Loads all songs from the given directory.
/// Returns an error if the path does not exist or is not a directory.
///
/// Synchronous version.
pub fn load_song_directory(
    path: &std::path::Path,
    cache: Option<&mut dyn SongCache>,
) -> Result<LoadedSongs, LoadSongError> {
    if !path.exists() || !path.is_dir() {
        return Err(LoadSongError::PathDoesNotExist(path.to_path_buf()));
    }

    // read_dir is fine here, we don't need recursion
    let loaded_songs: Vec<LoadedSong> = std::fs::read_dir(path)?
        .filter_map(|entry| {
            let entry = entry.ok()?;
            let path = entry.path();
            // we cache later in bulk
            let song_data = match load_song_from_path(entry.path(), None) {
                Ok(data) => data,
                Err(e) => {
                    warn!("Failed to load song from path {:?}: {}", path, e);
                    return None;
                }
            };

            Some(song_data)
        })
        .collect();

    // cache
    if let Some(c) = cache {
        c.cache_songs(loaded_songs.clone())?;
    }

    Ok(LoadedSongs {
        songs: loaded_songs,
    })
}

// TODO: Add progress reporting
/// Loads all songs from the given directory in parallel.
/// Returns an error if the path does not exist or is not a directory.
/// Parallel version.
pub fn load_song_directory_parallel(
    path: &Path,
    cache: Option<&mut dyn SongCache>,
) -> Result<LoadedSongs, LoadSongError> {
    if !path.exists() || !path.is_dir() {
        return Err(LoadSongError::PathDoesNotExist(path.to_path_buf()));
    }

    // Compute hashes in parallel but do not touch the mutable cache from the parallel closure.
    let loaded_songs: Vec<LoadedSong> = std::fs::read_dir(path)?
        .par_bridge()
        .filter_map(|entry| {
            let entry = entry.ok()?;
            let path = entry.path();
            match compute_custom_level_from_path(&path) {
                Ok(hash) => Some(LoadedSong { hash, path }),
                Err(e) => {
                    warn!("Failed to load song from path {:?}: {}", path, e);
                    None
                }
            }
        })
        .collect();

    // Cache songs sequentially to avoid moving a mutable reference into the parallel closure.
    if let Some(c) = cache {
        c.cache_songs(loaded_songs.clone())?;
    }

    Ok(LoadedSongs {
        songs: loaded_songs,
    })
}
