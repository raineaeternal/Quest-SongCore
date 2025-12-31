use std::path::PathBuf;

use serde::{Deserialize, Serialize};

use crate::models::beatmap::BeatmapLevelColorSchemeSaveData;

/// Represents a difficulty beatmap within a beatmap set (matches the C# `_difficulty*` fields)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DifficultyBeatmapV2 {
    #[serde(rename = "_difficulty")]
    pub difficulty: String,

    #[serde(rename = "_difficultyRank")]
    pub difficulty_rank: u32,

    #[serde(rename = "_beatmapFilename")]
    pub beatmap_filename: String,

    #[serde(rename = "_noteJumpMovementSpeed")]
    pub note_jump_movement_speed: f32,

    #[serde(rename = "_noteJumpStartBeatOffset")]
    pub note_jump_start_beat_offset: f32,

    #[serde(rename = "_beatmapColorSchemeIdx")]
    pub beatmap_color_scheme_idx: Option<usize>,

    #[serde(rename = "_environmentNameIdx")]
    pub environment_name_idx: Option<usize>,
}

/// Represents a characteristic -> beatmaps grouping (matches `_beatmapCharacteristicName` and `_difficultyBeatmaps`)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DifficultyBeatmapSetV2 {
    #[serde(rename = "_beatmapCharacteristicName")]
    pub beatmap_characteristic_name: String,

    #[serde(rename = "_difficultyBeatmaps")]
    pub difficulty_beatmaps: Vec<DifficultyBeatmapV2>,
}

/// Top-level StandardLevelInfoSaveData (version 2.x) matching the C# private field names
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct StandardLevelInfoSaveDataV2 {
    #[serde(rename = "_version")]
    pub version: Option<String>,

    #[serde(rename = "_songName")]
    pub song_name: Option<String>,

    #[serde(rename = "_songSubName")]
    pub song_sub_name: Option<String>,

    #[serde(rename = "_songAuthorName")]
    pub song_author_name: Option<String>,

    #[serde(rename = "_levelAuthorName")]
    pub level_author_name: Option<String>,

    #[serde(rename = "_beatsPerMinute")]
    pub beats_per_minute: f32,

    #[serde(rename = "_songTimeOffset")]
    pub song_time_offset: Option<f32>,

    #[serde(rename = "_shuffle")]
    pub shuffle: f32,

    #[serde(rename = "_shufflePeriod")]
    pub shuffle_period: f32,

    #[serde(rename = "_previewStartTime")]
    pub preview_start_time: f32,

    #[serde(rename = "_previewDuration")]
    pub preview_duration: f32,

    #[serde(rename = "_songFilename")]
    pub song_filename: PathBuf,

    #[serde(rename = "_coverImageFilename")]
    pub cover_image_filename: Option<PathBuf>,

    #[serde(rename = "_environmentName")]
    pub environment_name: String,

    #[serde(rename = "_allDirectionsEnvironmentName")]
    pub all_directions_environment_name: String,

    #[serde(rename = "_environmentNames")]
    pub environment_names: Vec<String>,

    #[serde(rename = "_colorSchemes")]
    pub color_schemes: Vec<BeatmapLevelColorSchemeSaveData>,

    #[serde(rename = "_difficultyBeatmapSets")]
    pub difficulty_beatmap_sets: Vec<DifficultyBeatmapSetV2>,
}
