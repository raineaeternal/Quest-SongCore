use std::path::PathBuf;

use serde::{Deserialize, Serialize};

use crate::models::{common::Color, info_dat::ColorScheme};

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
    pub song_filename: PathBuf,

    #[serde(rename = "songDuration")]
    pub song_duration: f32,

    #[serde(rename = "audioDataFilename")]
    pub audio_data_filename: Option<PathBuf>,

    #[serde(rename = "bpm")]
    pub bpm: f32,

    #[serde(rename = "lufs")]
    pub lufs: Option<f32>,

    #[serde(rename = "previewStartTime")]
    pub preview_start_time: f32,

    #[serde(rename = "previewDuration")]
    pub preview_duration: f32,
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

fn convert_htmlstring_to_color(color_html_string: &str) -> Color {
    if color_html_string.is_empty() {
        return Color::black();
    }

    let color_string = if color_html_string.starts_with('#') {
        color_html_string.to_string()
    } else {
        format!("#{color_html_string}")
    };
    // parse hex color #rrggbb
    let hex = u32::from_str_radix(&color_string[1..], 16).unwrap_or(0);
    let r = ((hex >> 16) & 0xFF) as u8;
    let g = ((hex >> 8) & 0xFF) as u8;
    let b = (hex & 0xFF) as u8;

    let r_normalized = (r as f32) / 255.0;
    let g_normalized = (g as f32) / 255.0;
    let b_normalized = (b as f32) / 255.0;

    Color {
        r: r_normalized,
        g: g_normalized,
        b: b_normalized,
        a: None,
    }

    // Color::from_hex_string(&color_string).unwrap_or(Color::black())
}

impl From<ColorSchemeV4> for ColorScheme {
    fn from(value: ColorSchemeV4) -> Self {
        ColorScheme {
            color_scheme_id: value.color_scheme_name.unwrap_or_default(),
            saber_a_color: convert_htmlstring_to_color(&value.saber_a_color.unwrap_or_default()),
            saber_b_color: convert_htmlstring_to_color(&value.saber_b_color.unwrap_or_default()),
            environment_color0: convert_htmlstring_to_color(
                &value.environment_color_0.unwrap_or_default(),
            ),
            environment_color1: convert_htmlstring_to_color(
                &value.environment_color_1.unwrap_or_default(),
            ),
            obstacles_color: convert_htmlstring_to_color(
                &value.obstacles_color.unwrap_or_default(),
            ),
            environment_color0_boost: convert_htmlstring_to_color(
                &value.environment_color_0_boost.unwrap_or_default(),
            ),
            environment_color1_boost: convert_htmlstring_to_color(
                &value.environment_color_1_boost.unwrap_or_default(),
            ),
        }
    }
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
    pub characteristic: String,

    #[serde(rename = "difficulty")]
    pub difficulty: String,

    #[serde(rename = "beatmapAuthors")]
    pub beatmap_authors: Option<BeatmapAuthorsV4>,

    #[serde(rename = "environmentNameIdx")]
    pub environment_name_idx: usize,

    #[serde(rename = "beatmapColorSchemeIdx")]
    pub beatmap_color_scheme_idx: Option<usize>,

    #[serde(rename = "noteJumpMovementSpeed")]
    pub note_jump_movement_speed: f32,

    #[serde(rename = "noteJumpStartBeatOffset")]
    pub note_jump_start_beat_offset: f32,

    #[serde(rename = "lightshowDataFilename")]
    pub lightshow_data_filename: PathBuf,

    #[serde(rename = "beatmapDataFilename")]
    pub beatmap_data_filename: PathBuf,
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
    pub song_preview_filename: PathBuf,

    #[serde(rename = "coverImageFilename")]
    pub cover_image_filename: Option<PathBuf>,

    #[serde(rename = "environmentNames")]
    pub environment_names: Option<Vec<String>>,

    #[serde(rename = "colorSchemes")]
    pub color_schemes: Option<Vec<ColorSchemeV4>>,

    #[serde(rename = "difficultyBeatmaps")]
    pub difficulty_beatmaps: Vec<DifficultyBeatmapV4>,
}
