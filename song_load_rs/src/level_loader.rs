use std::path::Path;

use std::collections::HashMap;

use crate::models::beatmap::{
    BeatmapBasicData, BeatmapCharacteristic, BeatmapDifficulty, FileSystemBeatmapLevelData,
    PlayerSensitivityFlag, PreviewMediaData,
};
use crate::models::{v2::StandardLevelInfoSaveDataV2, v4::BeatmapLevelSaveDataV4};

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

    custom_level_data: StandardLevelInfoSaveData,
    beatmap_level_data: FileSystemBeatmapLevelData,
    custom_level_path: String,
    //     CustomJSONData::CustomLevelInfoSaveDataV2* _customLevelSaveDataV2;
    // CustomJSONData::CustomBeatmapLevelSaveDataV4* _customBeatmapLevelSaveDataV4;
    // GlobalNamespace::IBeatmapLevelData* _beatmapLevelData;
    // std::string _customLevelPath;
}

impl CustomBeatmapLevel {
    // -1
    const K_INVALID_VERSION: u32 = u32::MAX;

    fn new(
        version: u32,
        custom_level_path: String,
        save: StandardLevelInfoSaveData,
        beatmap_level_data: FileSystemBeatmapLevelData,
        has_precalculated_data: bool,
        level_id: String,
        song_name: String,
        song_sub_name: String,
        song_author_name: String,
        all_mappers: Vec<String>,
        all_lighters: Vec<String>,
        bpm: f32,
        lufs: f32,
        song_time_offset: f32,
        preview_start_time: f32,
        preview_duration: f32,
        song_duration: Option<std::time::Duration>,
        content_rating: PlayerSensitivityFlag,
        into_ipreview_media_data: PreviewMediaData,
        beatmap_basic_data: HashMap<(BeatmapCharacteristic, BeatmapDifficulty), BeatmapBasicData>,
    ) -> Self {
        Self {
            version,
            has_precalculated_data,
            level_id,
            song_name,
            song_sub_name,
            song_author_name,
            all_mappers,
            all_lighters,
            beats_per_minute: bpm,
            integrated_lufs: lufs,
            song_time_offset,
            preview_start_time,
            preview_duration,
            song_duration: song_duration.map(|d| d.as_secs_f32()).unwrap_or_default(),
            content_rating,
            preview_media_data: into_ipreview_media_data,
            beatmap_basic_datas: beatmap_basic_data,
            custom_level_data: save,
            beatmap_level_data,
            custom_level_path,
        }
    }
}

#[derive(Debug, Clone)]
pub enum StandardLevelInfoSaveData {
    V2(StandardLevelInfoSaveDataV2),
    V4(BeatmapLevelSaveDataV4),
}

pub fn get_preview_media_data(
    level_path: &Path,
    cover_image_filename: Option<&Path>,
    song_filename: &Path,
) -> PreviewMediaData {
    // Adjust the constructor call below to match your PreviewMediaData API.
    // This mirrors: FileSystemPreviewMediaData::New_ctor(_spriteAsyncLoader, _clipLoader, levelPath.string(), coverImageFilename, songFilename)
    let cover_sprite =
        cover_image_filename.map(|cover| level_path.join(cover.file_name().unwrap()));
    PreviewMediaData {
        level_path: level_path.to_path_buf(),
        cover_sprite,
        preview_audio_clip: level_path.join(song_filename.file_name().unwrap()),
    }
}
