use std::path::{Path, PathBuf};

use rayon::iter::{ParallelBridge, ParallelIterator};
use tracing::warn;

use crate::{cache::SongCache, hash::compute_custom_level_from_path};

#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct LoadedSong {
    pub path: PathBuf,
    pub hash: String,
}

#[derive(Clone)]
pub struct LoadedSongs {
    pub songs: Vec<LoadedSong>,
}

pub fn load_song_from_path(
    path: PathBuf,
    mut cache: Option<&mut dyn SongCache>,
) -> Result<LoadedSong, String> {
    if !path.exists() {
        return Err("Path does not exist".to_owned());
    }

    let cached_hash = cache
        .as_mut()
        .map(|c| c.get_cached_song(&path))
        .transpose()
        .map_err(|s| format!("{s}"))?
        .flatten();

    // Return cached hash if available
    if let Some(cached) = cached_hash {
        return Ok(cached);
    }

    let hash = match compute_custom_level_from_path(&path.to_path_buf()) {
        Ok(hash) => hash,
        Err(e) => return Err(format!("Failed to compute hash: {}", e)),
    };

    if let Some(c) = cache {
        c.cache_song(LoadedSong {
            hash: hash.clone(),
            path: path.clone(),
        })
        .map_err(|s| format!("{s}"))?;
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
) -> Result<LoadedSongs, String> {
    if !path.exists() || !path.is_dir() {
        return Err("Path does not exist or is not a directory".to_owned());
    }

    // read_dir is fine here, we don't need recursion
    let loaded_songs: Vec<LoadedSong> = std::fs::read_dir(path)
        .map_err(|e| e.to_string())?
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
        c.cache_songs(loaded_songs.clone())
            .map_err(|s| format!("{s}"))?;
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
) -> Result<LoadedSongs, String> {
    if !path.exists() || !path.is_dir() {
        return Err("Path does not exist or is not a directory".to_owned());
    }

    // Compute hashes in parallel but do not touch the mutable cache from the parallel closure.
    let loaded_songs: Vec<LoadedSong> = std::fs::read_dir(path)
        .map_err(|e| e.to_string())?
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
        c.cache_songs(loaded_songs.clone())
            .map_err(|s| format!("{s}"))?;
    }

    Ok(LoadedSongs {
        songs: loaded_songs,
    })
}
