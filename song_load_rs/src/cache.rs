pub mod mem_cache;
pub mod file_cache;

use std::path::Path;

use thiserror::Error;

use crate::song_load::LoadedSong;

#[derive(Debug, Error)]
pub enum CacheError {
    #[error("General error: {0}")]
    GeneralError(String),
    #[error("I/O error: {0}")]
    IoError(std::io::Error),
}

pub trait SongCache: Send + Sync {
    /// Clears the entire song cache.
    fn clear_cache(&mut self) -> Result<(), CacheError>;

    /// Resets the cached data for the given song path.
    fn reset_song_cache(&mut self, song_path: &Path) -> Result<(), CacheError>;

    /// Caches the loaded song data for the given song path.
    fn cache_song(&mut self, loaded_song_data: LoadedSong) -> Result<(), CacheError>;

    fn cache_songs(&mut self, loaded_songs: Vec<LoadedSong>) -> Result<(), CacheError> {
        for song in loaded_songs {
            self.cache_song(song)?;
        }
        Ok(())
    }

    /// Retrieves the cached loaded song data for the given song path, if it exists.
    fn get_cached_song(&self, song_path: &Path) -> Result<Option<LoadedSong>, CacheError>;

    fn reload_cache(&mut self) -> Result<(), CacheError> {
        Ok(())
    }
    fn save_cache(&self) -> Result<(), CacheError> {
        Ok(())
    }
}
