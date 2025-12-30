use serde::Deserialize;

#[derive(Debug, Clone, Deserialize)]
pub struct Color {
    pub r: f32,
    pub g: f32,
    pub b: f32,
    pub a: Option<f32>,
}
