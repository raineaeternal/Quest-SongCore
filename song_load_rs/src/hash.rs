use std::{collections::HashMap, io};

use bytes::Bytes;
use sha1::{Digest, Sha1};

use std::path::PathBuf;

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

/// Compute the custom level hash from an in-memory map of files (path -> bytes).
///
/// The function looks for `Info.dat` (or `info.dat`) in the provided map, uses
/// its bytes as the `prepend` value, then hashes files in the following order:
/// 1. `_songFilename` (if present)
/// 2. Each `_beatmapFilename` found in `_difficultyBeatmapSets` in sets -> beatmaps order.
///
/// Only files present in the provided `files` map are included; missing files are skipped.
pub fn compute_custom_level_hash_from_info_dat(
    files: &HashMap<PathBuf, Bytes>,
) -> io::Result<String> {
    // find Info.dat / info.dat by file name
    let info_entry = files.iter().find(|(p, _)| {
        if let Some(name) = p.file_name().and_then(|s| s.to_str()) {
            name == "Info.dat" || name == "info.dat"
        } else {
            false
        }
    });

    let (_info_path, info_bytes) = match info_entry {
        Some((p, b)) => (p.clone(), b.clone()),
        None => {
            return Err(io::Error::new(
                io::ErrorKind::NotFound,
                "Info.dat not found in map",
            ));
        }
    };

    let info_contents = String::from_utf8(info_bytes.to_vec())
        .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;

    let info: InfoDat = serde_json::from_str(&info_contents)
        .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;

    let prepend_bytes = info_contents.as_bytes();

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

pub fn compute_custom_level_hash_from_zip(
    zip_archive: &mut zip::ZipArchive<std::io::Cursor<Bytes>>,
) -> io::Result<String> {
    // Read all files into a map of PathBuf -> Bytes
    let mut files_map: HashMap<PathBuf, Bytes> = HashMap::new();

    for i in 0..zip_archive.len() {
        let mut file = zip_archive.by_index(i)?;
        let mut buffer = Vec::with_capacity(file.size() as usize);
        io::copy(&mut file, &mut buffer)?;

        files_map.insert(PathBuf::from(file.name()), Bytes::from(buffer));
    }

    compute_custom_level_hash_from_info_dat(&files_map)
}
