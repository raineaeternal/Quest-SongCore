use std::{
    collections::HashMap,
    io::{self, Seek},
    path::Path,
};

use bytes::Bytes;
use sha1::{Digest, Sha1};
use zip::read::ZipFile;

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

pub fn necessary_files_from_info_dat(path: &Path, info: &InfoDat) -> Vec<PathBuf> {
    let mut necessary_files = Vec::new();

    if let Some(song_filename) = &info.song_filename {
        necessary_files.push(path.join(song_filename));
    }

    if let Some(beatmap_sets) = &info.difficulty_beatmap_sets {
        for set in beatmap_sets {
            for beatmap in &set.difficulty_beatmaps {
                necessary_files.push(path.join(&beatmap.beatmap_filename));
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

/// Compute the custom level hash from a zip archive.
///
/// The function looks for `Info.dat` (or `info.dat`) in the zip, uses
/// its bytes as the `prepend` value, then hashes files in the following order:
/// 1. `_songFilename` (if present)
/// 2. Each `_beatmapFilename` found in `_difficultyBeatmapSets` in sets -> beatmaps order.
///
/// Only files present in the zip are included; missing files are skipped.
pub fn compute_custom_level_hash_from_zip<R: std::io::Read + Seek>(
    zip_archive: &mut zip::ZipArchive<R>,
) -> io::Result<String> {
    let read_file = |mut v: ZipFile<R>| {
        let mut buffer = Vec::with_capacity(v.size() as usize);
        io::copy(&mut v, &mut buffer)?;
        Ok(Bytes::from(buffer))
    };

    let info_file1 = zip_archive.by_name("Info.dat").and_then(read_file);
    let info_file2 = zip_archive.by_name("info.dat").and_then(read_file);

    let info_buffer = info_file1.or(info_file2)?.to_vec();

    let info_contents = String::from_utf8(info_buffer)
        .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;
    let info_dat: InfoDat = serde_json::from_str(&info_contents)
        .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;

    let necessary_files: Vec<PathBuf> = necessary_files_from_info_dat(Path::new(""), &info_dat);
    let files_map = necessary_files
        .into_iter()
        .filter_map(|p| match zip_archive.by_name(p.to_str().unwrap()) {
            Ok(mut file) => {
                let mut buffer = Vec::with_capacity(file.size() as usize);
                if io::copy(&mut file, &mut buffer).is_err() {
                    return None;
                }
                Some((p, Bytes::from(buffer)))
            }
            Err(_) => None,
        })
        .collect::<HashMap<PathBuf, Bytes>>();

    // let mut files_map: HashMap<PathBuf, Bytes> = HashMap::new();
    // for i in 0..zip_archive.len() {
    //     let mut file = zip_archive.by_index(i)?;
    //     // skip files not in the necessary files
    //     if !files_map.contains_key(&PathBuf::from(file.name())) {
    //         continue;
    //     }
    //     let mut buffer = Vec::with_capacity(file.size() as usize);
    //     io::copy(&mut file, &mut buffer)?;

    //     files_map.insert(PathBuf::from(file.name()), Bytes::from(buffer));
    // }

    compute_custom_level_hash_from_info_dat(&files_map, &info_dat, info_contents.as_bytes())
}

/// Compute the custom level hash from a directory path.
/// The function looks for `Info.dat` (or `info.dat`) in the directory, uses
/// its bytes as the `prepend` value, then hashes files in the following order:
/// 1. `_songFilename` (if present)
/// 2. Each `_beatmapFilename` found in `_difficultyBeatmapSets` in sets -> beatmaps order.
///
/// Only files present in the directory are included; missing files are skipped.
pub fn compute_custom_level_hash_from_directory(path: &Path) -> Result<String, io::Error> {
    // Read files from directory into a map of PathBuf -> Bytes
    let info_dat_path = {
        let info_dat1 = path.join("Info.dat");
        let info_dat2 = path.join("info.dat");

        if info_dat1.exists() {
            Some(info_dat1)
        } else if info_dat2.exists() {
            Some(info_dat2)
        } else {
            None
        }
    };

    if info_dat_path.is_none() {
        return Err(io::Error::new(
            io::ErrorKind::NotFound,
            "Info.dat not found in directory",
        ));
    }

    let info_bytes = std::fs::read(info_dat_path.unwrap())?;
    let info_contents =
        String::from_utf8(info_bytes).map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;
    let info_dat: InfoDat = serde_json::from_str(&info_contents)
        .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;

    let files_map: HashMap<PathBuf, Bytes> = necessary_files_from_info_dat(path, &info_dat)
        .into_iter()
        .filter_map(|p| {
            let full_path = path.join(&p);
            if !(full_path.exists() && full_path.is_file()) {
                return None;
            }

            match std::fs::read(&full_path) {
                Ok(bytes) => Some((p, Bytes::from(bytes))),
                Err(_) => None,
            }
        })
        .collect();

    compute_custom_level_hash_from_info_dat(&files_map, &info_dat, info_contents.as_bytes())
}

/// Compute the custom level hash from a file path or directory path.
/// If the path is a file, it is treated as a zip archive.
/// If the path is a directory, it is treated as a custom level directory.
/// Returns an error if the path does not exist.
pub fn compute_custom_level_from_path(path: &Path) -> io::Result<String> {
    if !path.exists() {
        return Err(io::Error::new(
            io::ErrorKind::NotFound,
            "Path does not exist",
        ));
    }

    if path.is_file() {
        // Open as zip archive
        let file_bytes = std::fs::read(path)?;
        let cursor = std::io::Cursor::new(Bytes::from(file_bytes));
        let mut zip_archive = zip::ZipArchive::new(cursor)?;

        return compute_custom_level_hash_from_zip(&mut zip_archive);
    }

    // Otherwise, treat as directory
    compute_custom_level_hash_from_directory(path)
}
