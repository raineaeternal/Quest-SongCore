use std::{collections::HashMap, path::Path};

use tracing::warn;

use crate::{level_loader::{BeatmapCharacteristicSO, IBeatmapLevelData}, models::{beatmap::BeatmapBasicData, v2::{self, StandardLevelInfoSaveDataV2}}};

// V2 | V3
/// Rough Rust translation of the original GetBeatmapLevelAndBasicData.
/// This implementation follows the original control flow and error handling (warnings & skips).
/// It returns a placeholder IBeatmapLevelData and a map of basic data keyed by (characteristic, difficulty).
pub fn get_beatmap_level_and_basic_data_v2(
    level_path: &Path,
    level_id: &str,
    save_data: &v2::StandardLevelInfoSaveDataV2,
    environment_names: &[String],
    color_schemes: &[Option<String>],
) -> (
    IBeatmapLevelData,
    HashMap<(BeatmapCharacteristicSO, BeatmapDifficulty), BeatmapBasicData>,
) {
    let mut file_difficulty_beatmaps: HashMap<
        (BeatmapCharacteristicSO, BeatmapDifficulty),
        String,
    > = HashMap::new();
    let mut basic_data_dict: HashMap<
        (BeatmapCharacteristicSO, BeatmapDifficulty),
        BeatmapBasicData,
    > = HashMap::new();

    for beatmap_set in save_data.difficulty_beatmap_sets.iter().flatten() {
        // placeholder: resolve characteristic by serialized name
        let characteristic = match beatmap_characteristics::get_by_serialized_name(
            &beatmap_set.beatmap_characteristic_name,
        ) {
            Some(c) => c,
            None => {
                warn!(
                    "Got null characteristic for characteristic name {}, skipping...",
                    beatmap_set.beatmap_characteristic_name
                );
                continue;
            }
        };

        for difficulty_beatmap in beatmap_set.difficulty_beatmaps.iter().flatten() {
            // parse difficulty string into BeatmapDifficulty (placeholder parse)
            let difficulty = match crate::beatmap_difficulty::from_serialized_name(
                &difficulty_beatmap.difficulty,
            ) {
                Some(d) => d,
                None => {
                    warn!(
                        "Failed to parse a diff string: {}, skipping...",
                        difficulty_beatmap.difficulty
                    );
                    continue;
                }
            };

            let beatmap_filename = match &difficulty_beatmap.beatmap_filename {
                Some(s) => s,
                None => {
                    warn!("Beatmap entry missing filename, skipping...");
                    continue;
                }
            };
            let beatmap_path = level_path.join(beatmap_filename);
            if !beatmap_path.exists() {
                warn!(
                    "Diff file '{}' does not exist, skipping...",
                    beatmap_path.display()
                );
                continue;
            }

            let key = (characteristic.clone(), difficulty.clone());
            if file_difficulty_beatmaps.contains_key(&key) {
                warn!(
                    "Duplicate characteristic/difficulty: {}/{}",
                    beatmap_set.beatmap_characteristic_name,
                    format!("{:?}", difficulty)
                );
                continue;
            }

            file_difficulty_beatmaps
                .insert(key.clone(), beatmap_path.to_string_lossy().to_string());

            // compute envNameIndex
            let save_data_had_env_names =
                !save_data.environment_names.unwrap_or_default().is_empty();

            let mut env_name_index = if save_data_had_env_names {
                difficulty_beatmap.environment_name_idx
            } else if crate::beatmap_characteristics::contains_rotation_events(&characteristic) {
                1
            } else {
                0
            };
            // clamp to valid range
            env_name_index = env_name_index.clamp(0, environment_names.len().saturating_sub(1));

            let color_scheme = difficulty_beatmap
                .beatmap_color_scheme_idx
                .and_then(|idx| color_schemes.get(idx as usize).cloned())
                .flatten();

            // Build BeatmapBasicData -- here we fill with a default placeholder.
            // In a full implementation map the concrete fields (njm, offset, env name, color scheme, etc).
            basic_data_dict.insert(
                key,
                BeatmapBasicData::new(
                    difficulty_beatmap.note_jump_movement_speed,
                    difficulty_beatmap.note_jump_start_beat_offset,
                    environment_names.get(env_name_index_usize).cloned(),
                    color_scheme,
                ),
            );
        }
    }

    // Construct a placeholder IBeatmapLevelData representing the filesystem level data.
    // A full implementation would create a FileSystemBeatmapLevelData-like structure with the collected dict.
    let beatmap_level_data = IBeatmapLevelData;

    (beatmap_level_data, basic_data_dict)
}

/// Reads info.dat (or Info.dat) from the given path and tries to deserialize as v2 (StandardLevelInfoSaveDataV2).
pub fn get_save_data_from_v2(path: &Path) -> Option<v2::StandardLevelInfoSaveDataV2> {
    if path.as_os_str().is_empty() {
        warn!("Provided path was empty!");
        return None;
    }

    let info_path = if path.join("info.dat").exists() {
        path.join("info.dat")
    } else if path.join("Info.dat").exists() {
        path.join("Info.dat")
    } else {
        warn!(
            "no info.dat found for song @ '{}', returning null!",
            path.display()
        );
        return None;
    };

    match std::fs::read_to_string(&info_path) {
        Ok(text) => match serde_json::from_str::<v2::StandardLevelInfoSaveDataV2>(&text) {
            Ok(data) => Some(data),
            Err(e) => {
                warn!("Cannot parse info.dat {}: {}", info_path.display(), e);
                None
            }
        },
        Err(e) => {
            warn!(
                "Cannot load file from path: {}! error: {}",
                path.display(),
                e
            );
            None
        }
    }
}

pub fn basic_verify_map_v2(level_path: &Path, save_data: &StandardLevelInfoSaveDataV2) -> bool {
    let song_file = &save_data.song_filename;
    let cover_file = &save_data.cover_image_filename;

    if let Some(song_file) = song_file
        && !level_path.join(song_file).exists()
    {
        return false;
    }
    if let Some(cover_file) = cover_file
        && !level_path.join(cover_file).exists()
    {
        return false;
    }

    for set in save_data.difficulty_beatmap_sets.iter().flatten() {
        for diff in set.difficulty_beatmaps.iter().flatten() {
            if diff.beatmap_filename.is_none()
                || !level_path
                    .join(&diff.beatmap_filename.as_ref().unwrap())
                    .exists()
            {
                return false;
            }
        }
    }

    true
}
