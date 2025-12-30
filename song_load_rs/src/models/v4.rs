use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SongDataV4 {
    #[serde(rename = "title")]
    pub title: Option<String>,

    #[serde(rename = "subTitle")]
    pub sub_title: Option<String>,

    #[serde(rename = "author")]
    pub author: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AudioDataV4 {
    #[serde(rename = "songFilename")]
    pub song_filename: String,

    #[serde(rename = "songDuration")]
    pub song_duration: Option<f32>,

    #[serde(rename = "audioDataFilename")]
    pub audio_data_filename: Option<String>,

    #[serde(rename = "bpm")]
    pub bpm: Option<f32>,

    #[serde(rename = "lufs")]
    pub lufs: Option<f32>,

    #[serde(rename = "previewStartTime")]
    pub preview_start_time: Option<f32>,

    #[serde(rename = "previewDuration")]
    pub preview_duration: Option<f32>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ColorSchemeV4 {
    #[serde(rename = "colorSchemeName")]
    pub color_scheme_name: Option<String>,

    #[serde(rename = "overrideNotes")]
    pub override_notes: Option<bool>,

    #[serde(rename = "saberAColor")]
    pub saber_a_color: Option<String>,

    #[serde(rename = "saberBColor")]
    pub saber_b_color: Option<String>,

    #[serde(rename = "obstaclesColor")]
    pub obstacles_color: Option<String>,

    #[serde(rename = "overrideLights")]
    pub override_lights: Option<bool>,

    #[serde(rename = "environmentColor0")]
    pub environment_color_0: Option<String>,

    #[serde(rename = "environmentColor1")]
    pub environment_color_1: Option<String>,

    #[serde(rename = "environmentColor0Boost")]
    pub environment_color_0_boost: Option<String>,

    #[serde(rename = "environmentColor1Boost")]
    pub environment_color_1_boost: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BeatmapAuthorsV4 {
    #[serde(rename = "mappers")]
    pub mappers: Option<Vec<String>>,

    #[serde(rename = "lighters")]
    pub lighters: Option<Vec<String>>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DifficultyBeatmapV4 {
    #[serde(rename = "characteristic")]
    pub characteristic: Option<String>,

    #[serde(rename = "difficulty")]
    pub difficulty: Option<String>,

    #[serde(rename = "beatmapAuthors")]
    pub beatmap_authors: Option<BeatmapAuthorsV4>,

    #[serde(rename = "environmentNameIdx")]
    pub environment_name_idx: Option<i32>,

    #[serde(rename = "beatmapColorSchemeIdx")]
    pub beatmap_color_scheme_idx: Option<i32>,

    #[serde(rename = "noteJumpMovementSpeed")]
    pub note_jump_movement_speed: Option<f32>,

    #[serde(rename = "noteJumpStartBeatOffset")]
    pub note_jump_start_beat_offset: Option<f32>,

    #[serde(rename = "lightshowDataFilename")]
    pub lightshow_data_filename: Option<String>,

    #[serde(rename = "beatmapDataFilename")]
    pub beatmap_data_filename: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BeatmapLevelSaveDataV4 {
    #[serde(rename = "version")]
    pub version: String,

    #[serde(rename = "song")]
    pub song: SongDataV4,

    #[serde(rename = "audio")]
    pub audio: AudioDataV4,

    #[serde(rename = "songPreviewFilename")]
    pub song_preview_filename: Option<String>,

    #[serde(rename = "coverImageFilename")]
    pub cover_image_filename: Option<String>,

    #[serde(rename = "environmentNames")]
    pub environment_names: Option<Vec<String>>,

    #[serde(rename = "colorSchemes")]
    pub color_schemes: Option<Vec<ColorSchemeV4>>,

    #[serde(rename = "difficultyBeatmaps")]
    pub difficulty_beatmaps: Option<Vec<DifficultyBeatmapV4>>,
}
