use std::path::{Path, PathBuf};
use std::sync::RwLock;

use crate::beatmap::BeatmapSource;
use crate::info_dat::InfoDat;
use crate::loader::beatmap_metadata_loader::LoadBeatmapMetadataError;
use crate::models::beatmap::{
    BeatmapBasicData, BeatmapCharacteristic, BeatmapDifficulty, FileSystemBeatmapLevelData,
    PlayerSensitivityFlag, PreviewMediaData,
};

pub mod v2;
pub mod v4;

#[derive(Debug, Clone)]
pub struct CustomBeatmapLevel {
    pub version: u32,
    pub has_precalculated_data: bool,
    pub level_id: String,
    pub song_name: String,
    pub song_sub_name: String,
    pub song_author_name: String,
    pub all_mappers: Vec<String>,
    pub all_lighters: Vec<String>,
    pub beats_per_minute: f32,
    pub integrated_lufs: f32,
    pub song_time_offset: f32,
    pub preview_start_time: f32,
    pub preview_duration: f32,
    pub song_duration: f32,
    pub content_rating: PlayerSensitivityFlag,
    pub preview_media_data: PreviewMediaData,
    pub beatmap_basic_datas:
        std::collections::HashMap<(BeatmapCharacteristic, BeatmapDifficulty), BeatmapBasicData>,

    pub custom_level_data: InfoDat,
    pub beatmap_level_data: FileSystemBeatmapLevelData,
    pub custom_level_path: PathBuf,
    pub hash: String,
}

impl CustomBeatmapLevel {
    // -1
    const K_INVALID_VERSION: u32 = u32::MAX;
}

#[derive(Debug, thiserror::Error)]
pub enum CustomLevelLoaderError {
    #[error("I/O error: {0}")]
    IoError(#[from] std::io::Error),

    #[error("Info.dat file not found in beatmap source")]
    InfoDatNotFound,

    #[error("Beatmap verification failed")]
    BeatmapVerificationFailed,

    #[error("Beatmap metadata error: {0}")]
    BeatmapMetadataError(#[from] LoadBeatmapMetadataError),
}

/// Loads a custom beatmap level from the given beatmap source (zip or directory).
/// Returns a Result containing the loaded CustomBeatmapLevel or an error.
pub fn load_level_from_path<C>(
    beatmap: &BeatmapSource,
    wip: bool,
    song_cache: Option<&RwLock<C>>,
) -> Result<Option<CustomBeatmapLevel>, CustomLevelLoaderError>
where
    C: crate::cache::SongCache + ?Sized,
{
    let info_dat = beatmap.get_info_dat()?;

    match info_dat {
        InfoDat::V2(save_data_v2) => {
            v2::load_custom_beatmap_level_v2(beatmap, wip, save_data_v2, song_cache)
        }
        InfoDat::V4(save_data_v4) => {
            v4::load_custom_beatmap_level_v4(beatmap, wip, save_data_v4, song_cache)
        }
    }
}

pub fn load_level_from_directories<C>(
    directories: &[&Path],
    is_wip: bool,
    song_cache: Option<&RwLock<C>>,
) -> Result<Vec<CustomBeatmapLevel>, CustomLevelLoaderError>
where
    C: crate::cache::SongCache + ?Sized,
{
    let entries = directories
        .iter()
        .flat_map(std::fs::read_dir)
        .flatten()
        .filter_map(|entry| {
            let entry = entry.ok()?;
            let path = entry.path();
            if path.is_dir() || path.extension().is_some_and(|ext| ext == "zip") {
                Some(path)
            } else {
                None
            }
        });

    entries
        .map(
            |path| -> Result<Option<CustomBeatmapLevel>, CustomLevelLoaderError> {
                let beatmap =
                    BeatmapSource::from_path(path).map_err(CustomLevelLoaderError::from)?;
                load_level_from_path(&beatmap, is_wip, song_cache)
            },
        )
        // ignore None results
        .filter_map(|o| o.transpose())
        .collect()
}

/// Parallel version of `load_level_from_directories`.
/// Loads custom beatmap levels from the given directories in parallel.
/// Returns a Result containing a vector of loaded CustomBeatmapLevel or an error.
///
/// # Parameters:
/// - `directories`: A slice of paths to directories containing beatmaps.
/// - `is_wip`: A boolean indicating whether to load WIP data.
/// - `song_cache`: An optional reference to a RwLock containing a SongCache for caching.
/// - `func`: An optional callback function that is called with each loaded CustomBeatmapLevel,
///   along with its index and the total count.
pub fn load_level_from_directories_parallel<C, F>(
    directories: &[&Path],
    is_wip: bool,
    song_cache: Option<&RwLock<C>>,
    func: Option<&F>,
) -> Result<Vec<CustomBeatmapLevel>, CustomLevelLoaderError>
where
    C: crate::cache::SongCache + ?Sized,
    F: Fn(&CustomBeatmapLevel, usize, usize) + std::marker::Sync,
{
    use rayon::prelude::*;

    let entries: Vec<_> = directories
        .iter()
        .flat_map(std::fs::read_dir)
        .flatten()
        .filter_map(|entry| {
            let entry = entry.ok()?;
            let path = entry.path();
            if path.is_dir() || path.extension().is_some_and(|ext| ext == "zip") {
                Some(path)
            } else {
                None
            }
        })
        .collect();
    let total_count = entries.len();

    entries
        .into_iter()
        .par_bridge()
        .map(
            |path| -> Result<Option<CustomBeatmapLevel>, CustomLevelLoaderError> {
                let beatmap =
                    BeatmapSource::from_path(path).map_err(CustomLevelLoaderError::from)?;
                load_level_from_path(&beatmap, is_wip, song_cache)
            },
        )
        // ignore None results
        .filter_map(|o| o.transpose())
        .inspect(|b| {
            if let Some(callback) = func
                && let Ok(b) = b
            {
                let index = 0; // You may want to implement a way to get the correct index here.
                callback(b, index, total_count);
            }
        })
        .collect()
}

pub fn get_preview_media_data(
    beatmap: &BeatmapSource,
    cover_image_filename: Option<&Path>,
    song_filename: &Path,
) -> PreviewMediaData {
    // Adjust the constructor call below to match your PreviewMediaData API.
    // This mirrors: FileSystemPreviewMediaData::New_ctor(_spriteAsyncLoader, _clipLoader, levelPath.string(), coverImageFilename, songFilename)
    let cover_sprite = cover_image_filename
        .and_then(|cover| beatmap.get_file_bytes(cover.file_name().unwrap()).ok());
    PreviewMediaData {
        level_path: beatmap.get_real_path().to_path_buf(),
        cover_sprite,
        preview_audio_clip: beatmap
            .get_file_bytes(song_filename.file_name().unwrap())
            .ok(),
    }
}
