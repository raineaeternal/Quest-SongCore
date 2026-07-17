use std::{path::PathBuf, time::Duration};

use serde::{Deserialize, Serialize};

/// A struct representing useful cached data for a loaded song.
#[derive(Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Hash, Serialize, Deserialize)]
pub struct BeatmapMetadata {
    /// Path to the beatmap
    pub path: PathBuf,
    /// SHA1 hash of the beatmap
    pub hash: String,
    /// Optional length of the song if known
    pub song_length: Option<Duration>,
}

#[derive(Clone)]
pub struct BeatmapMetadataArray {
    pub songs: Vec<BeatmapMetadata>,
}
