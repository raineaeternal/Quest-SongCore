pub type SongCoreMap<K, V> = ahash::AHashMap<K, V>;
pub type SongCoreLock<T> = std::sync::RwLock<T>;
pub type SongCoreAsyncLock<T> = tokio::sync::RwLock<T>;