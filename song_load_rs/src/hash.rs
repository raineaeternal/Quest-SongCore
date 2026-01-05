use std::io::{self};

use bytes::Bytes;
use sha1::{Digest, Sha1};

use std::path::PathBuf;

use crate::{beatmap::BeatmapSource, info_dat::InfoDat};

/// Compute a SHA-1 from an ordered iterator of `(PathBuf, Bytes)` pairs.
/// The order of the iterator is respected (no internal sorting).
pub fn create_sha1_from_path_bytes<I>(prepend_bytes: &[u8], files: I) -> io::Result<String>
where
    I: IntoIterator<Item = Bytes>,
{
    let mut hasher = Sha1::new_with_prefix(prepend_bytes);
    // hasher.update(prepend_bytes);

    for bytes in files.into_iter() {
        hasher.update(&bytes);
    }

    Ok(format!("{:x}", hasher.finalize()))
}

/// Get the list of necessary files from the InfoDat for hashing.
pub fn necessary_files_from_info_dat(info: &InfoDat) -> Vec<PathBuf> {
    let mut necessary_files: Vec<PathBuf> = Vec::with_capacity(10);

    // if let Some(song_filename) = info.get_song_filename() {
    //     necessary_files.push(song_filename.into());
    // }

    if let InfoDat::V4(v4) = info {
        // V4+ includes cover image in hash

        if let Some(audio_data) = v4.audio.audio_data_filename.clone() {
            necessary_files.push(audio_data);
        }
    }

    necessary_files.extend(
        info.get_beatmap_files()
            .map(|p| p.to_path_buf())
            .collect::<Vec<_>>(),
    );

    necessary_files
}

/// Compute the custom level hash from a `Beatmap` (zip or directory).
#[tracing::instrument(level = "trace", skip(beatmap))]
pub fn compute_custom_level_hash_from_beatmap(beatmap: &BeatmapSource) -> io::Result<String> {
    // Read Info.dat/info.dat bytes via Beatmap helper
    let (info_bytes, info_dat) = beatmap.get_info_dat()?;

    let necessary_files: Vec<PathBuf> = necessary_files_from_info_dat(&info_dat);


    // Collect beatmap files in the order from the InfoDat, skipping missing entries.
    let path_bytes: Vec<Bytes> = necessary_files
        .into_iter()
        .filter(|p| beatmap.has_file(p))
        .map(|p| beatmap.get_file_bytes(p))
        .collect::<Result<_, _>>()?;

    create_sha1_from_path_bytes(&info_bytes, path_bytes)
}
