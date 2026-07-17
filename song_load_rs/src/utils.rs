use std::sync::RwLock;

pub type SongCoreLock<T> = RwLock<T>;
pub type SongCoreMap<K, V> = ahash::AHashMap<K, V>;
// pub type SongCoreMap<K, V> = rustc_hash::FxHashMap<K, V>; 
