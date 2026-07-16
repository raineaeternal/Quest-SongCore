use serde::{Deserialize, Serialize};

use crate::models::common::Color;

#[cfg(feature = "info-dat")]
pub mod v2;
#[cfg(feature = "info-dat")]
pub mod v4;

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
