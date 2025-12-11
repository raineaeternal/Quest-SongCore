use std::path::PathBuf;

use song_load_rs::cache::SongCache;

// Integration tests for the in-memory song cache implementation `MemCache`.

#[test]
fn mem_cache_basic_operations() -> Result<(), Box<dyn std::error::Error>> {
    // Construct the mem cache
    let mut cache = song_load_rs::cache::mem_cache::MemCache::default();

    // Prepare a sample LoadedSong
    let path = PathBuf::from("/tmp/test_song");
    let song = song_load_rs::song_load::LoadedSong {
        path: path.clone(),
        hash: "deadbeef".to_string(),
        song_length: None,
    };

    // Initially the cache should not contain the song
    let got = cache.get_cached_song(&path)?;
    assert!(got.is_none(), "cache unexpectedly contained an entry");

    // Cache the song
    cache.cache_song(song.clone())?;

    // Now retrieval should return our song
    let got = cache.get_cached_song(&path)?;
    assert!(got.is_some(), "cached song not found");
    let got = got.unwrap();
    assert_eq!(got.hash, "deadbeef");
    assert_eq!(got.path, path);

    // Reset the cached song
    cache.reset_song_cache(&path)?;
    let got = cache.get_cached_song(&path)?;
    assert!(got.is_none(), "song was not removed by reset_song_cache");

    // Cache multiple songs then clear
    let s1 = song_load_rs::song_load::LoadedSong {
        path: PathBuf::from("/tmp/a"),
        hash: "a".into(),
        song_length: None,
    };
    let s2 = song_load_rs::song_load::LoadedSong {
        path: PathBuf::from("/tmp/b"),
        hash: "b".into(),
        song_length: None,
    };
    cache.cache_song(s1)?;
    cache.cache_song(s2)?;

    cache.clear_cache()?;
    let none1 = cache.get_cached_song(&PathBuf::from("/tmp/a"))?;
    let none2 = cache.get_cached_song(&PathBuf::from("/tmp/b"))?;
    assert!(
        none1.is_none() && none2.is_none(),
        "clear_cache did not remove entries"
    );

    Ok(())
}
