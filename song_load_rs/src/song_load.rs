use std::path::PathBuf;

use rayon::iter::{ParallelBridge, ParallelIterator};
use tracing::warn;

pub struct LoadedSong {
    pub path: PathBuf,
    pub hash: String,
}

pub struct LoadedSongs {
    pub songs: Vec<LoadedSong>,
}

pub fn load_song_from_path(path: PathBuf) -> Result<LoadedSong, String> {
    if !path.exists() {
        return Err("Path does not exist".to_owned());
    }

    let hash = match crate::hash::compute_custom_level_from_path(&path.to_path_buf()) {
        Ok(hash) => hash,
        Err(e) => return Err(format!("Failed to compute hash: {}", e)),
    };

    Ok(LoadedSong { hash, path })
}

/// Loads all songs from the given directory.
/// Returns an error if the path does not exist or is not a directory.
///
/// Synchronous version.
pub fn load_song_directory(path: &std::path::Path) -> Result<LoadedSongs, String> {
    if !path.exists() || !path.is_dir() {
        return Err("Path does not exist or is not a directory".to_owned());
    }

    // read_dir is fine here, we don't need recursion
    let loaded_songs = std::fs::read_dir(path)
        .map_err(|e| e.to_string())?
        .filter_map(|entry| {
            let entry = entry.ok()?;
            let path = entry.path();
            let song_data = match load_song_from_path(entry.path()) {
                Ok(data) => data,
                Err(e) => {
                    warn!("Failed to load song from path {:?}: {}", path, e);
                    return None;
                }
            };

            Some(song_data)
        })
        .collect();

    Ok(LoadedSongs {
        songs: loaded_songs,
    })
}

// TODO: Add progress reporting
/// Loads all songs from the given directory in parallel.
/// Returns an error if the path does not exist or is not a directory.
/// Parallel version.
pub fn load_song_directory_parallel<F>(path: &std::path::Path) -> Result<LoadedSongs, String> {
    if !path.exists() || !path.is_dir() {
        return Err("Path does not exist or is not a directory".to_owned());
    }

    let loaded_songs: Vec<LoadedSong> = std::fs::read_dir(path)
        .map_err(|e| e.to_string())?
        .par_bridge()
        .filter_map(|entry| {
            let entry = entry.ok()?;
            let path = entry.path();
            let song_data = match load_song_from_path(entry.path()) {
                Ok(data) => data,
                Err(e) => {
                    warn!("Failed to load song from path {:?}: {}", path, e);
                    return None;
                }
            };

            Some(song_data)
        })
        .collect();

    Ok(LoadedSongs {
        songs: loaded_songs,
    })
}
