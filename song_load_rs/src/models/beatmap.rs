use std::path::PathBuf;

use serde::{Deserialize, Serialize};

use crate::models::common::Color;

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct BeatmapLevelColorSchemeSaveData {
    #[serde(rename = "colorScheme")]
    pub color_scheme: ColorScheme,
    #[serde(rename = "useOverride")]
    pub use_override: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub struct ColorScheme {
    #[serde(rename = "colorSchemeId")]
    pub color_scheme_id: String,

    #[serde(rename = "saberAColor")]
    pub saber_a_color: Color,

    #[serde(rename = "saberBColor")]
    pub saber_b_color: Color,

    #[serde(rename = "environmentColor0")]
    pub environment_color0: Color,

    #[serde(rename = "environmentColor1")]
    pub environment_color1: Color,

    #[serde(rename = "obstaclesColor")]
    pub obstacles_color: Color,

    #[serde(rename = "environmentColor0Boost")]
    pub environment_color0_boost: Color,

    #[serde(rename = "environmentColor1Boost")]
    pub environment_color1_boost: Color,
}

#[derive(Debug, Clone)]
pub struct EnvironmentName(pub String);

#[derive(Debug, Clone)]
pub struct BeatmapBasicData {
    pub note_jump_movement_speed: f32,
    pub note_jump_start_beat_offset: f32,
    pub environment: Option<EnvironmentName>,
    pub color_scheme: Option<ColorScheme>,
    pub notes_count: Option<u32>,
    pub cuttable_objects_count: Option<u32>,
    pub obstacles_count: Option<u32>,
    pub bombs_count: Option<u32>,

    pub mappers: Vec<String>,
    pub lighters: Vec<String>,
}
impl BeatmapBasicData {}
#[derive(Debug, Clone)]
pub struct BeatmapKey;

#[derive(Debug, Clone)]
pub enum PlayerSensitivityFlag {
    Unknown,
    Safe,
    Themes,
    Explicit,
}

#[derive(Debug, Clone)]
pub struct PreviewMediaData {
    pub level_path: PathBuf,
    pub cover_sprite: Option<PathBuf>,
    pub preview_audio_clip: PathBuf,
}
/// Represents a beatmap characteristic (e.g., Standard, OneSaber, etc.)
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct BeatmapCharacteristic(pub String);

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct BeatmapDifficulty(pub String);

#[derive(Debug, Clone, PartialEq)]
pub struct FileSystemBeatmapLevelData {
    pub audio_clip_path: PathBuf,
    pub audio_data_path: Option<PathBuf>,
    pub name: String,
    pub difficulty_beatmaps: std::collections::HashMap<
        (BeatmapCharacteristic, BeatmapDifficulty),
        FileDifficultyBeatmap,
    >,
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct FileDifficultyBeatmap {
    pub beatmap_path: PathBuf,
    pub lightshow_path: Option<PathBuf>,
}
