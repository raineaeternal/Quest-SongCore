use std::path::PathBuf;

use crate::{
    cache::{CacheError, SongCache},
    models::metadata::BeatmapMetadata,
    utils::SongCoreMap,
};

#[derive(Debug, Default)]
pub struct MemCache {
    cache: SongCoreMap<PathBuf, BeatmapMetadata>,
}

impl MemCache {
    pub fn new() -> Self {
        Self {
            cache: SongCoreMap::default(),
        }
    }
    pub fn with_capacity(capacity: usize) -> Self {
        Self {
            cache: SongCoreMap::with_capacity_and_hasher(capacity, Default::default()),
        }
    }
    pub fn from_cache(cache: SongCoreMap<PathBuf, BeatmapMetadata>) -> Self {
        Self { cache }
    }

    pub fn get_cache(&self) -> &SongCoreMap<PathBuf, BeatmapMetadata> {
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

    fn cache_song(&mut self, loaded_song_data: BeatmapMetadata) -> Result<(), CacheError> {
        self.cache
            .insert(loaded_song_data.path.clone(), loaded_song_data);
        Ok(())
    }

    fn get_cached_song(
        &self,
        song_path: &std::path::Path,
    ) -> Result<Option<BeatmapMetadata>, CacheError> {
        Ok(self.cache.get(song_path).cloned())
    }
}
