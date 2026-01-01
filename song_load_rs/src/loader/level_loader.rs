use std::path::{Path, PathBuf};

use crate::beatmap::BeatmapSource;
use crate::info_dat::InfoDat;
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
    //     CustomJSONData::CustomLevelInfoSaveDataV2* _customLevelSaveDataV2;
    // CustomJSONData::CustomBeatmapLevelSaveDataV4* _customBeatmapLevelSaveDataV4;
    // GlobalNamespace::IBeatmapLevelData* _beatmapLevelData;
    // std::string _customLevelPath;
}

impl CustomBeatmapLevel {
    // -1
    const K_INVALID_VERSION: u32 = u32::MAX;
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
