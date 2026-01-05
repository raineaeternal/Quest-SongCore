use std::{
    io::{self},
};

use ahash::AHashMap;
use bytes::Bytes;
use sha1::{Digest, Sha1};

use std::path::PathBuf;

use crate::{beatmap::BeatmapSource, info_dat::InfoDat};

/// Compute a SHA-1 from an ordered iterator of `(PathBuf, Bytes)` pairs.
/// The order of the iterator is respected (no internal sorting).
pub fn create_sha1_from_path_bytes<I>(prepend_bytes: &[u8], files: I) -> io::Result<String>
where
    I: IntoIterator<Item = (PathBuf, Bytes)>,
{
    let mut hasher = Sha1::new();
    hasher.update(prepend_bytes);

    for (_path, bytes) in files.into_iter() {
        hasher.update(&bytes);
    }

    Ok(format!("{:x}", hasher.finalize()))
}

/// Get the list of necessary files from the InfoDat for hashing.
pub fn necessary_files_from_info_dat(info: &InfoDat) -> Vec<PathBuf> {
    let mut necessary_files: Vec<PathBuf> = Vec::new();

    if let Some(song_filename) = info.get_song_filename() {
        necessary_files.push(song_filename.into());
    }

    necessary_files.extend(
        info.get_beatmap_files()
            .map(|p| p.to_path_buf())
            .collect::<Vec<_>>(),
    );

    necessary_files
}

/// Compute the custom level hash from an in-memory map of files (path -> bytes).
///
/// The function looks for `Info.dat` (or `info.dat`) in the provided map, uses
/// its bytes as the `prepend` value, then hashes files in the following order:
/// 1. `_songFilename` (if present)
/// 2. Each `_beatmapFilename` found in `_difficultyBeatmapSets` in sets -> beatmaps order.
///
/// Only files present in the provided `files` map are included; missing files are skipped.
fn compute_custom_level_hash_from_info_dat(
    files: &AHashMap<PathBuf, Bytes>,
    info: &InfoDat,
    info_bytes: &[u8],
) -> io::Result<String> {
    let prepend_bytes = info_bytes;

    // Collect beatmap files in the order from the InfoDat, skipping missing entries.
    let mut path_bytes: Vec<(PathBuf, Bytes)> = Vec::new();
    for p in info.get_beatmap_files() {
        let pb = p.to_path_buf();
        let Some(b) = files.get(&pb) else { continue };

        path_bytes.push((pb, b.clone()));
    }

    create_sha1_from_path_bytes(prepend_bytes, path_bytes)
}

/// Compute the custom level hash from a `Beatmap` (zip or directory).
pub fn compute_custom_level_hash_from_beatmap(beatmap: &BeatmapSource) -> io::Result<String> {
    // Read Info.dat/info.dat bytes via Beatmap helper
    let info_bytes = beatmap.get_info_dat_bytes()?;
    let info_dat: InfoDat = beatmap.get_info_dat()?;
    let info_vec = info_bytes.to_vec();

    let info_contents = String::from_utf8(info_vec.clone())
        .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;

    let necessary_files: Vec<PathBuf> = necessary_files_from_info_dat(&info_dat);

    let mut files_map: AHashMap<PathBuf, Bytes> = AHashMap::new();
    for p in necessary_files {
        let Ok(bytes) = beatmap.get_file_bytes(&p) else {
            continue;
        };

        files_map.insert(p, bytes);
    }

    compute_custom_level_hash_from_info_dat(&files_map, &info_dat, info_contents.as_bytes())
}
