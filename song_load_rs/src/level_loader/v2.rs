use std::{collections::HashMap, path::Path};

use tracing::warn;

use crate::{
    cache::SongCache,
    level_loader::{CustomBeatmapLevel, StandardLevelInfoSaveData, get_preview_media_data},
    models::{
        beatmap::{
            BeatmapBasicData, BeatmapCharacteristic, BeatmapDifficulty,
            BeatmapLevelColorSchemeSaveData, EnvironmentName, FileDifficultyBeatmap,
            FileSystemBeatmapLevelData,
        },
        v2::{self, StandardLevelInfoSaveDataV2},
    },
};

use crate::song_loader::{self, CUSTOM_LEVEL_PREFIX_ID};

// V2 | V3
/// Rough Rust translation of the original GetBeatmapLevelAndBasicData.
/// This implementation follows the original control flow and error handling (warnings & skips).
/// It returns a placeholder IBeatmapLevelData and a map of basic data keyed by (characteristic, difficulty).
pub fn get_beatmap_level_and_basic_data_v2(
    level_path: &Path,
    level_id: &str,
    save_data: &v2::StandardLevelInfoSaveDataV2,
    environment_names: &[EnvironmentName],
    color_schemes: &[BeatmapLevelColorSchemeSaveData],
) -> (
    FileSystemBeatmapLevelData,
    HashMap<(BeatmapCharacteristic, BeatmapDifficulty), BeatmapBasicData>,
) {
    let mut file_difficulty_beatmaps: HashMap<
        (BeatmapCharacteristic, BeatmapDifficulty),
        FileDifficultyBeatmap,
    > = HashMap::new();
    let mut basic_data_dict: HashMap<(BeatmapCharacteristic, BeatmapDifficulty), BeatmapBasicData> =
        HashMap::new();

    for beatmap_set in save_data.difficulty_beatmap_sets.iter() {
        // placeholder: resolve characteristic by serialized name
        let characteristic = BeatmapCharacteristic(beatmap_set.beatmap_characteristic_name.clone());

        for difficulty_beatmap in beatmap_set.difficulty_beatmaps.iter() {
            // parse difficulty string into BeatmapDifficulty (placeholder parse)
            let difficulty = BeatmapDifficulty(difficulty_beatmap.difficulty.clone());
            let beatmap_filename = &difficulty_beatmap.beatmap_filename;

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

            file_difficulty_beatmaps.insert(
                key.clone(),
                FileDifficultyBeatmap {
                    beatmap_path,
                    lightshow_path: None, // V2 does not have lightshow data
                },
            );

            // compute envNameIndex
            let _save_data_had_env_names = !save_data.environment_names.is_empty();

            let env_name_index = difficulty_beatmap
                .environment_name_idx
                // clamp to valid range;
                .map(|i| i.clamp(0, environment_names.len().saturating_sub(1)));

            let color_scheme = difficulty_beatmap
                .beatmap_color_scheme_idx
                .and_then(|i| color_schemes.get(i))
                .map(|c| &c.color_scheme)
                .cloned();

            let environment = env_name_index.map(|e| environment_names[e].clone());

            // Build BeatmapBasicData -- here we fill with a default placeholder.
            // In a full implementation map the concrete fields (njm, offset, env name, color scheme, etc).
            basic_data_dict.insert(
                key,
                BeatmapBasicData {
                    note_jump_movement_speed: difficulty_beatmap.note_jump_movement_speed,
                    note_jump_start_beat_offset: difficulty_beatmap.note_jump_start_beat_offset,
                    environment,
                    color_scheme,
                    notes_count: None,
                    cuttable_objects_count: None,
                    obstacles_count: None,
                    bombs_count: None,
                    mappers: vec![],
                    lighters: vec![],
                },
            );
        }
    }

    // Construct a placeholder IBeatmapLevelData representing the filesystem level data.
    // A full implementation would create a FileSystemBeatmapLevelData-like structure with the collected dict.
    let beatmap_level_data = FileSystemBeatmapLevelData {
        audio_clip_path: level_path.join(&save_data.song_filename),
        audio_data_path: None,
        name: level_id.to_string(),
        difficulty_beatmaps: file_difficulty_beatmaps,
    };

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

    if !level_path.join(song_file).exists() {
        return false;
    }
    if let Some(cover_file) = cover_file
        && !level_path.join(cover_file).exists()
    {
        return false;
    }

    for set in save_data.difficulty_beatmap_sets.iter() {
        for diff in set.difficulty_beatmaps.iter() {
            if !level_path.join(&diff.beatmap_filename).exists() {
                return false;
            }
        }
    }

    true
}

/// Convert a v2 info JSON string into the internal Custom (here represented as the v2 struct itself)
pub fn load_custom_save_data_v2(json_str: &str) -> Option<v2::StandardLevelInfoSaveDataV2> {
    match serde_json::from_str::<v2::StandardLevelInfoSaveDataV2>(json_str) {
        Ok(s) => Some(s),
        Err(e) => {
            warn!("Failed to parse V2 save data: {}", e);
            None
        }
    }
}

/// Rust translation of `LevelLoader::LoadCustomBeatmapLevel` (V2)
/// Returns `None` on error or missing data.
pub fn load_custom_beatmap_level_v2(
    level_path: &Path,
    wip: bool,
    save_data: Option<v2::StandardLevelInfoSaveDataV2>,
    song_cache: &mut impl SongCache,
) -> Option<(CustomBeatmapLevel, String)> {
    let save = match save_data {
        Some(s) => s,
        None => {
            warn!("saveData was null for level @ {}", level_path.display());
            if cfg!(feature = "throw_on_missing_data") {
                panic!("saveData was null for level @ {}", level_path.display());
            }
            return None;
        }
    };

    if !basic_verify_map_v2(level_path, &save) {
        warn!("Map {} was missing files!", level_path.display());
        if cfg!(feature = "throw_on_missing_data") {
            panic!("Map {} was missing files!", level_path.display());
        }
        return None;
    }

    let song_data =
        song_loader::load_song_from_path(level_path.to_path_buf(), Some(song_cache)).ok()?;

    let level_id = format!(
        "{CUSTOM_LEVEL_PREFIX_ID}{}{}",
        song_data.hash,
        if wip { " WIP" } else { "" }
    );

    let song_name = save.song_name.clone().unwrap_or_default();
    let song_sub_name = save.song_sub_name.clone().unwrap_or_default();
    let song_author_name = save.song_author_name.clone().unwrap_or_default();
    let level_author_name = save.level_author_name.clone().unwrap_or_default();

    let bpm = save.beats_per_minute;
    let song_time_offset = save.song_time_offset.unwrap_or_default();
    let preview_start_time = save.preview_start_time;
    let preview_duration = save.preview_duration;

    // Build environment names list
    let environment_infos: Vec<EnvironmentName> = save
        .environment_names
        .clone()
        .into_iter()
        .map(EnvironmentName)
        .collect();

    let environment_list: Vec<EnvironmentName> = if environment_infos.is_empty() {
        let env = EnvironmentName(save.environment_name.clone());
        let all_dirs = EnvironmentName(save.all_directions_environment_name.clone());
        vec![env, all_dirs]
    } else {
        environment_infos
    };

    // color schemes
    let color_schemes = &save.color_schemes;

    let song_duration = None; // length resolution handled by song cache / loader

    let all_mappers = vec![level_author_name.clone()];
    let all_lighters: Vec<String> = Vec::new();

    // prepare preview media data paths
    let cover_path = save.cover_image_filename.as_deref();
    let song_file_path = save.song_filename.as_path();

    let preview_media_data = get_preview_media_data(level_path, cover_path, song_file_path);

    let (beatmap_level_data, beatmap_basic_data) = get_beatmap_level_and_basic_data_v2(
        level_path,
        &level_id,
        &save,
        &environment_list,
        color_schemes,
    );

    if beatmap_basic_data.is_empty() {
        return None;
    }

    let result = CustomBeatmapLevel::new(
        CustomBeatmapLevel::K_INVALID_VERSION,
        level_path.to_string_lossy().to_string(),
        StandardLevelInfoSaveData::V2(save),
        beatmap_level_data,
        false,
        level_id.clone(),
        song_name,
        song_sub_name,
        song_author_name,
        all_mappers,
        all_lighters,
        bpm,
        -6.0_f32,
        song_time_offset,
        preview_start_time,
        preview_duration,
        song_duration,
        crate::models::beatmap::PlayerSensitivityFlag::Safe,
        preview_media_data,
        beatmap_basic_data,
    );

    Some((result, song_data.hash))
}
