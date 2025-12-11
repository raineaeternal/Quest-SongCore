use std::{
    collections::HashMap,
    io::{self},
};

use bytes::Bytes;
use sha1::{Digest, Sha1};

use std::path::PathBuf;

use crate::beatmap::Beatmap;
use crate::models::InfoDat;

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

pub fn necessary_files_from_info_dat(info: &InfoDat) -> Vec<PathBuf> {
    let mut necessary_files = Vec::new();

    if let Some(song_filename) = &info.song_filename {
        necessary_files.push(song_filename.clone().into());
    }

    if let Some(beatmap_sets) = &info.difficulty_beatmap_sets {
        for set in beatmap_sets {
            for beatmap in &set.difficulty_beatmaps {
                necessary_files.push(beatmap.beatmap_filename.clone().into());
            }
        }
    }

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
    files: &HashMap<PathBuf, Bytes>,
    info: &InfoDat,
    info_bytes: &[u8],
) -> io::Result<String> {
    let prepend_bytes = info_bytes;

    // Build a fast lookup from basename -> (PathBuf, Bytes) to avoid repeated scans.
    let mut lookup: std::collections::HashMap<String, (PathBuf, Bytes)> = files
        .iter()
        .filter_map(|(p, b)| {
            p.file_name()
                .and_then(|s| s.to_str())
                .map(|name| (name.to_string(), (p.clone(), b.clone())))
        })
        .collect();

    // Create an iterator over filenames in the required order: only beatmaps in sets->beatmaps order.
    // Tests expect the "beatmaps-only" variant (no `_songFilename`/audio file included).
    let beatmap_filenames = info
        .difficulty_beatmap_sets
        .clone()
        .unwrap_or_default()
        .into_iter()
        .flat_map(|set| {
            set.difficulty_beatmaps
                .into_iter()
                .map(|b| b.beatmap_filename)
        });

    let filenames_iter = beatmap_filenames;

    // Map filenames to (PathBuf, Bytes) using the lookup; skip missing entries.
    let path_bytes_iter = filenames_iter.filter_map(move |name| lookup.remove(&name));

    create_sha1_from_path_bytes(prepend_bytes, path_bytes_iter)
}

/// Compute the custom level hash from a `Beatmap` (zip or directory).
pub fn compute_custom_level_hash_from_beatmap(beatmap: &Beatmap) -> io::Result<String> {
    // Read Info.dat/info.dat bytes via Beatmap helper
    let info_bytes = beatmap.get_info_dat_bytes()?;
    let info_vec = info_bytes.to_vec();

    let info_contents = String::from_utf8(info_vec.clone())
        .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;
    let info_dat: InfoDat = serde_json::from_str(&info_contents)
        .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;

    let necessary_files: Vec<PathBuf> = necessary_files_from_info_dat(&info_dat);

    let mut files_map: HashMap<PathBuf, Bytes> = HashMap::new();
    for p in necessary_files {
        let Ok(bytes) = beatmap.get_file_bytes(&p) else {
            continue;
        };

        files_map.insert(p, bytes);
    }

    compute_custom_level_hash_from_info_dat(&files_map, &info_dat, info_contents.as_bytes())
}
