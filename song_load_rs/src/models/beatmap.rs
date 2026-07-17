use std::path::PathBuf;

use bytes::Bytes;

use crate::models::info_dat::ColorScheme;

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
#[repr(C)]
pub enum PlayerSensitivityFlag {
    Unknown,
    Safe,
    Themes,
    Explicit,
}

/// Represents preview media data for a beatmap level.
#[derive(Debug, Clone)]
pub struct PreviewMediaData {
    pub level_path: PathBuf,
    pub cover_sprite: Option<Bytes>,
    pub preview_audio_clip: Option<Bytes>,
}
/// Represents a beatmap characteristic (e.g., Standard, OneSaber, etc.)
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct BeatmapCharacteristic(pub String);

/// Represents a beatmap difficulty (e.g., Easy, Normal, Hard, etc.)
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct BeatmapDifficulty(pub String);

/// Represents the data of a beatmap level stored in the file system.
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

/// Represents a specific difficulty beatmap file within a beatmap level.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct FileDifficultyBeatmap {
    pub beatmap_path: PathBuf,
    pub lightshow_path: Option<PathBuf>,
}
