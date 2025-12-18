use std::{io::Read, path::{Path, PathBuf}};

use crate::cache::{SongCache, mem_cache::MemCache};

#[derive(Debug)]
pub struct FileCache {
    pub path: PathBuf,
    pub mem_cache: MemCache,
}

type CacheData = std::collections::HashMap<PathBuf, crate::song_load::LoadedSong>;

impl FileCache {
    pub fn new(path: PathBuf) -> Self {
        Self {
            path,
            mem_cache: MemCache::default(),
        }
    }

    pub fn load_from_file(&mut self) -> Result<(), std::io::Error> {
        let file = std::fs::File::open(&self.path)?;
        let mut reader = std::io::BufReader::new(file);
        // reading from string is faster than from reader directly
        let mut str = String::new();

        reader.read_to_string(&mut str)?;
        let cache_data: CacheData = serde_json::from_str(&str)?;

        self.mem_cache = MemCache::from_cache(cache_data);
        Ok(())
    }

    pub fn save_to_file(&self) -> Result<(), std::io::Error> {
        let file = std::fs::File::create(&self.path)?;
        let writer = std::io::BufWriter::new(file);

        let cache = self.mem_cache.get_cache();
        serde_json::to_writer_pretty(writer, cache)?;

        Ok(())
    }
    
}

impl SongCache for FileCache {
    fn clear_cache(&mut self) -> Result<(), crate::cache::CacheError> {
        self.mem_cache.clear_cache()
    }

    fn reset_song_cache(&mut self, song_path: &Path) -> Result<(), crate::cache::CacheError> {
        self.mem_cache.reset_song_cache(song_path)
    }

    fn cache_song(&mut self, loaded_song_data: crate::song_load::LoadedSong) -> Result<(), crate::cache::CacheError> {
        self.mem_cache.cache_song(loaded_song_data)
    }

    fn cache_songs(&mut self, loaded_songs: Vec<crate::song_load::LoadedSong>) -> Result<(), super::CacheError> {
        self.mem_cache.cache_songs(loaded_songs)
    }

    fn get_cached_song(&self, song_path: &Path) -> Result<Option<crate::song_load::LoadedSong>, crate::cache::CacheError> {
        self.mem_cache.get_cached_song(song_path)
    }

    fn reload_cache(&mut self) -> Result<(), crate::cache::CacheError> {
        self.load_from_file().map_err(crate::cache::CacheError::IoError)
    }

    fn save_cache(&self) -> Result<(), super::CacheError> {
        self.save_to_file().map_err(crate::cache::CacheError::IoError)
    }
}