use std::{collections::HashMap, path::PathBuf};

use crate::{
    cache::{CacheError, SongCache},
    song_load::LoadedSong,
};

#[derive(Debug, Default)]
pub struct MemCache {
    cache: HashMap<PathBuf, LoadedSong>,
}

impl MemCache {
    pub fn new() -> Self {
        Self {
            cache: HashMap::new(),
        }
    }
    pub fn with_capacity(capacity: usize) -> Self {
        Self {
            cache: HashMap::with_capacity(capacity),
        }
    }
    pub fn from_cache(cache: HashMap<PathBuf, LoadedSong>) -> Self {
        Self { cache }
    }

    pub fn get_cache(&self) -> &HashMap<PathBuf, LoadedSong> {
        &self.cache
    }
}

impl SongCache for MemCache {
    fn clear_cache(&mut self) -> std::result::Result<(), CacheError> {
        self.cache.clear();
        Ok(())
    }

    fn reset_song_cache(&mut self, song_path: &std::path::Path) -> Result<(), CacheError> {
        self.cache.remove(song_path);
        Ok(())
    }

    fn cache_song(&mut self, loaded_song_data: LoadedSong) -> Result<(), CacheError> {
        self.cache
            .insert(loaded_song_data.path.clone(), loaded_song_data);
        Ok(())
    }

    fn get_cached_song(
        &self,
        song_path: &std::path::Path,
    ) -> Result<Option<LoadedSong>, CacheError> {
        Ok(self.cache.get(song_path).cloned())
    }
}
