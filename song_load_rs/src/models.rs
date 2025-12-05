use serde::Deserialize;


#[derive(Debug, Deserialize)]
pub struct DifficultyBeatmap {
    #[serde(rename = "_beatmapFilename")]
    pub beatmap_filename: String,
}

#[derive(Debug, Deserialize)]
pub struct DifficultyBeatmapSet {
    #[serde(rename = "_difficultyBeatmaps")]
    pub difficulty_beatmaps: Vec<DifficultyBeatmap>,
}

#[derive(Debug, Deserialize)]
pub struct InfoDat {
    #[serde(rename = "_songFilename")]
    pub song_filename: Option<String>,
    #[serde(rename = "_difficultyBeatmapSets")]
    pub difficulty_beatmap_sets: Option<Vec<DifficultyBeatmapSet>>,
}