use std::path::Path;

use crate::song_load::LoadedSong;

pub trait SongCache: Send + Sync {
    /// Clears the entire song cache.
    fn clear_cache(&mut self);

    /// Resets the cached data for the given song path.
    fn reset_song_cache(&mut self, song_path: &Path) -> Result<(), Box<dyn std::error::Error>>;

    /// Caches the loaded song data for the given song path.
    fn cache_song(
        &mut self,
        loaded_song_data: LoadedSong,
    ) -> Result<(), Box<dyn std::error::Error>>;

    fn cache_songs(
        &mut self,
        loaded_songs: Vec<LoadedSong>,
    ) -> Result<(), Box<dyn std::error::Error>> {
        for song in loaded_songs {
            self.cache_song(song)?;
        }
        Ok(())
    }

    /// Retrieves the cached loaded song data for the given song path, if it exists.
    fn get_cached_song(
        &self,
        song_path: &Path,
    ) -> Result<Option<LoadedSong>, Box<dyn std::error::Error>>;
}
