use std::path::Path;

use semver::Version;
use serde_json::Value as JsonValue;
use std::collections::HashMap;
use std::hash::{Hash, Hasher};
use tracing::warn;

use crate::models::beatmap::{BeatmapBasicData, BeatmapCharacteristicSO, BeatmapDifficulty, BeatmapKey, IBeatmapLevelData, IPreviewMediaData, PlayerSensitivityFlag};
use crate::{
    cache::SongCache,
    models::{v2::StandardLevelInfoSaveDataV2, v4::BeatmapLevelSaveDataV4},
    song_loader::{self, CUSTOM_LEVEL_PREFIX_ID},
};

pub mod v2;
pub mod v4;



#[derive(Debug, Clone)]
pub struct CustomBeatmapLevel {
    pub version: i32,
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
    pub preview_media_data: IPreviewMediaData,
    pub beatmap_basic_datas:
        std::collections::HashMap<(BeatmapCharacteristicSO, BeatmapDifficulty), BeatmapBasicData>,
    pub characteristics_cache: Option<Vec<BeatmapCharacteristicSO>>,
    pub beatmap_keys_cache: Option<Vec<BeatmapKey>>,

    custom_level_data: StandardLevelInfoSaveData,
    beatmap_level_data: IBeatmapLevelData,
    custom_level_path: String,
    //     CustomJSONData::CustomLevelInfoSaveDataV2* _customLevelSaveDataV2;
    // CustomJSONData::CustomBeatmapLevelSaveDataV4* _customBeatmapLevelSaveDataV4;
    // GlobalNamespace::IBeatmapLevelData* _beatmapLevelData;
    // std::string _customLevelPath;
}

impl CustomBeatmapLevel {
    const K_INVALID_VERSION: i32 = -1;
}

#[derive(Debug, Clone)]
pub enum StandardLevelInfoSaveData {
    V2(StandardLevelInfoSaveDataV2),
    V4(BeatmapLevelSaveDataV4),
}

impl PartialEq for BeatmapCharacteristicSO {
    fn eq(&self, other: &Self) -> bool {
        format!("{:?}", self) == format!("{:?}", other)
    }
}
impl Eq for BeatmapCharacteristicSO {}
impl Hash for BeatmapCharacteristicSO {
    fn hash<H: Hasher>(&self, state: &mut H) {
        format!("{:?}", self).hash(state);
    }
}

impl PartialEq for BeatmapDifficulty {
    fn eq(&self, other: &Self) -> bool {
        format!("{:?}", self) == format!("{:?}", other)
    }
}
impl Eq for BeatmapDifficulty {}
impl Hash for BeatmapDifficulty {
    fn hash<H: Hasher>(&self, state: &mut H) {
        format!("{:?}", self).hash(state);
    }
}

impl Default for BeatmapBasicData {
    fn default() -> Self {
        BeatmapBasicData
    }
}
