use std::path::Path;

use crate::models::info_dat::{v2::StandardLevelInfoSaveDataV2, v4::BeatmapLevelSaveDataV4};

/// Represents the Info.dat data structure, supporting multiple versions.
#[derive(Debug, Clone)]
pub enum InfoDat {
    V2(StandardLevelInfoSaveDataV2),
    V4(BeatmapLevelSaveDataV4),
}

impl InfoDat {
    /// Returns the song filename from the InfoDat, if available.
    pub fn get_song_filename(&self) -> Option<&Path> {
        match self {
            InfoDat::V2(data) => Some(&data.song_filename),
            InfoDat::V4(data) => Some(&data.audio.song_filename),
        }
    }

    /// Returns an iterator over all beatmap file paths referenced in the InfoDat.
    pub fn get_beatmap_files(&self) -> Box<dyn Iterator<Item = &Path> + '_> {
        match self {
            InfoDat::V2(data) => Box::new(
                data.difficulty_beatmap_sets
                    .iter()
                    .flat_map(|set| set.difficulty_beatmaps.iter())
                    .map(|beatmap| Path::new(&beatmap.beatmap_filename)),
            ),
            InfoDat::V4(data) => Box::new(data.difficulty_beatmaps.iter().flat_map(|beatmap| {
                [
                    Path::new(&beatmap.beatmap_data_filename),
                    Path::new(&beatmap.lightshow_data_filename),
                ]
            })),
        }
    }
}
