#include "CustomJSONData.hpp"
#include "hooking.hpp"
#include "logging.hpp"
#include "tasks.hpp"

#include "Utils/File.hpp"

#include "System/Collections/Generic/Dictionary_2.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmap.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/PreviewDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapLevelData.hpp"
#include "BeatmapSaveDataVersion3/BeatmapSaveData.hpp"
#include "GlobalNamespace/BeatmapDataLoader.hpp"
#include "GlobalNamespace/BeatmapDataBasicInfo.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/BeatmapDifficultySerializedMethods.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/AudioClipAsyncLoader.hpp"
#include "GlobalNamespace/HMCache_2.hpp"

#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "paper/shared/utfcpp/source/utf8.h"

#include <exception>
#include <filesystem>
#include <string_view>
#include <thread>

using namespace SongCore;
using namespace GlobalNamespace;
using namespace BeatmapSaveDataVersion3;

using std::filesystem::path;

std::pair<BeatmapSaveData*, BeatmapDataBasicInfo*> LoadBeatmapDataBasicInfo(path const& levelPath, StringW fileName, CustomJSONData::CustomLevelInfoSaveData* saveData) {
    auto cppFileName = utf8::utf16tou8(static_cast<std::u16string_view>(fileName));
    auto diffPath = levelPath / cppFileName;

    if (std::filesystem::exists(diffPath)) {
        try {
            auto beatmapSaveData = BeatmapSaveData::DeserializeFromJSONString(SongCore::Utils::ReadText(diffPath));
            auto beatmapDataBasicInfo = BeatmapDataLoader::GetBeatmapDataBasicInfoFromSaveData(beatmapSaveData);
            return {beatmapSaveData, beatmapDataBasicInfo};
        } catch (std::exception const& e) {
            ERROR("Error thrown while getting basic beatmap info: {}", e.what());
        } catch (...) {
            ERROR("Unknown error thrown while getting basic beatmap info: {}", typeid(std::current_exception()).name());
        }
    } else {
        WARNING("Diff file @ {} was expected but did not exist!", diffPath.string());
    }

    return {};
}

CustomDifficultyBeatmap* LoadDifficultyBeatmap(path const& levelPath, CustomBeatmapLevel* level, CustomDifficultyBeatmapSet* parentSet, CustomJSONData::CustomLevelInfoSaveData* saveData, CustomJSONData::CustomDifficultyBeatmap* difficultyBeatmap) {
    auto [beatmapSaveData, beatmapDataBasicInfo] = LoadBeatmapDataBasicInfo(levelPath, difficultyBeatmap->beatmapFilename, saveData);
    if (!beatmapSaveData || !beatmapDataBasicInfo) return nullptr;

    BeatmapDifficulty difficulty;
    BeatmapDifficultySerializedMethods::BeatmapDifficultyFromSerializedName(difficultyBeatmap->difficulty, byref(difficulty));

    return CustomDifficultyBeatmap::New_ctor(
        level->i___GlobalNamespace__IBeatmapLevel(),
        parentSet->i___GlobalNamespace__IDifficultyBeatmapSet(),
        difficulty,
        difficultyBeatmap->difficultyRank,
        difficultyBeatmap->noteJumpMovementSpeed,
        difficultyBeatmap->noteJumpStartBeatOffset,
        saveData->beatsPerMinute,
        difficultyBeatmap->beatmapColorSchemeIdx,
        difficultyBeatmap->environmentNameIdx,
        beatmapSaveData,
        beatmapDataBasicInfo->i___GlobalNamespace__IBeatmapDataBasicInfo()
    );
}

IDifficultyBeatmapSet* LoadDifficultyBeatmapSet(path const& levelPath, CustomBeatmapLevel* level, CustomJSONData::CustomLevelInfoSaveData* saveData, GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmapSet* difficultySet, GlobalNamespace::PreviewDifficultyBeatmapSet* previewSet) {
    auto beatmapSet = CustomDifficultyBeatmapSet::New_ctor(previewSet->beatmapCharacteristic);
    auto saveDataSets = saveData->difficultyBeatmapSets;

    ArrayW<CustomDifficultyBeatmap*> difficultyBeatmaps(previewSet->beatmapDifficulties.size());
    for (int i = -1; auto originalBeatmap : difficultySet->get_difficultyBeatmaps()) {
        i++;
        auto beatmap = il2cpp_utils::try_cast<CustomJSONData::CustomDifficultyBeatmap>(originalBeatmap).value_or(nullptr);
        if (!beatmap) continue;

        difficultyBeatmaps[i] = LoadDifficultyBeatmap(levelPath, level, beatmapSet, saveData, beatmap);
    }

    beatmapSet->SetCustomDifficultyBeatmaps(difficultyBeatmaps);

    return beatmapSet->i___GlobalNamespace__IDifficultyBeatmapSet();
}

ListW<IDifficultyBeatmapSet*> LoadDifficultyBeatmapSets(path const& levelPath, CustomBeatmapLevel* level, CustomPreviewBeatmapLevel* previewLevel, CustomJSONData::CustomLevelInfoSaveData* saveData) {
    auto sets = ListW<IDifficultyBeatmapSet*>::New();

    auto previewSets = ListW<GlobalNamespace::PreviewDifficultyBeatmapSet*>(previewLevel->previewDifficultyBeatmapSets);
    for (auto difficultySet : saveData->difficultyBeatmapSets) {
        auto previewSetItr = std::find_if(previewSets.begin(), previewSets.end(), [difficultySet](auto x){ return x->beatmapCharacteristic->serializedName == difficultySet->get_beatmapCharacteristicName(); });
        if (previewSetItr == previewSets.end()) continue;
        auto set = LoadDifficultyBeatmapSet(levelPath, level, saveData, difficultySet, *previewSetItr);
        if (set) sets->Add(set);
    }

    return sets;
}

BeatmapLevelData* LoadBeatmapLevelData(BeatmapLevelsModel* beatmapLevelsModel, path const& levelPath, CustomBeatmapLevel* level, CustomPreviewBeatmapLevel* previewLevel, CustomJSONData::CustomLevelInfoSaveData* saveData) {
    using namespace std::chrono_literals;
    auto sets = LoadDifficultyBeatmapSets(levelPath, level, previewLevel, saveData);
    if (!sets) return nullptr;

    // loadsong needs to happen on main thread or something because it loads an audioclip
    std::atomic<System::Threading::Tasks::Task_1<UnityW<UnityEngine::AudioClip>>*> atomicTask = nullptr;
    BSML::MainThreadScheduler::Schedule([&atomicTask, level, beatmapLevelsModel](){
        atomicTask = beatmapLevelsModel->_audioClipAsyncLoader->LoadSong(level->i___GlobalNamespace__IBeatmapLevel());
    });
    while (!atomicTask) std::this_thread::sleep_for(100ms);

    auto task = (System::Threading::Tasks::Task_1<UnityW<UnityEngine::AudioClip>>*)atomicTask;
    while(!task->IsCompleted) std::this_thread::sleep_for(100ms);
    if (!task->IsCompletedSuccessfully) return nullptr;
    auto clip = task->Result;
    return BeatmapLevelData::New_ctor(clip, sets->i___System__Collections__Generic__IReadOnlyList_1_T_());
}

CustomBeatmapLevel* LoadCustomBeatmapLevel(BeatmapLevelsModel* beatmapLevelsModel, CustomPreviewBeatmapLevel* previewBeatmapLevel) {
    auto levelPath = std::filesystem::path(previewBeatmapLevel->get_customLevelPath());
    if (!std::filesystem::exists(levelPath) || !std::filesystem::is_directory(levelPath)) return nullptr;

    auto saveData = il2cpp_utils::try_cast<CustomJSONData::CustomLevelInfoSaveData>(previewBeatmapLevel->standardLevelInfoSaveData).value_or(nullptr);
    auto level = CustomBeatmapLevel::New_ctor(previewBeatmapLevel);
    auto levelData = LoadBeatmapLevelData(beatmapLevelsModel, levelPath, level, previewBeatmapLevel, saveData);

    level->SetBeatmapLevelData(levelData);
    return level;
}

MAKE_AUTO_HOOK_ORIG_MATCH(BeatmapLevelsModel_GetBeatmapLevelAsync, &BeatmapLevelsModel::GetBeatmapLevelAsync, SongCore::Task<GlobalNamespace::BeatmapLevelsModel::GetBeatmapLevelResult>*, BeatmapLevelsModel* self, StringW levelID, SongCore::CancellationToken cancellationToken) {
    auto orig = BeatmapLevelsModel_GetBeatmapLevelAsync(self, levelID, cancellationToken);
    if (levelID.starts_with("custom_level_")) { // check if this is a custom level first
        return SongCore::StartTask<BeatmapLevelsModel::GetBeatmapLevelResult>([=](SongCore::CancellationToken cancelToken){
            using namespace std::chrono_literals;
            while(!orig->IsCompleted) std::this_thread::sleep_for(10ms);

            auto result = orig->Result;
            // if we get a valid result, just return that
            if (!result.isError || cancelToken.IsCancellationRequested) return result;

            GlobalNamespace::IPreviewBeatmapLevel* previewBeatmapLevel = nullptr;
            if (!self->_loadedPreviewBeatmapLevels->TryGetValue(levelID, byref(previewBeatmapLevel))) return result;
            auto customPreviewBeatmapLevel = il2cpp_utils::try_cast<GlobalNamespace::CustomPreviewBeatmapLevel>(previewBeatmapLevel).value_or(nullptr);
            if (!customPreviewBeatmapLevel) return result;

            auto customBeatmapLevel = LoadCustomBeatmapLevel(self, customPreviewBeatmapLevel);
            if (!customBeatmapLevel) return result;

            BSML::MainThreadScheduler::Schedule([=](){
                try {
                    self->_loadedBeatmapLevels->PutToCache(levelID, customBeatmapLevel->i___GlobalNamespace__IBeatmapLevel());
                } catch(std::exception& e) {
                    ERROR("Putting to cache threw exception {}, with what: {}", typeid(e).name(), e.what());
                } catch(...) {
                    ERROR("Putting to cache threw unknown exception {}", typeid(std::current_exception()).name());
                }
            });

            return GlobalNamespace::BeatmapLevelsModel::GetBeatmapLevelResult(false, customBeatmapLevel->i___GlobalNamespace__IBeatmapLevel());
        }, std::forward<SongCore::CancellationToken>(cancellationToken));
    } else { // if not a custom level just return orig
        return orig;
    }
}

// hook to fix the song duration not applying, which the game just doesn't copy for some reason
MAKE_HOOK_MATCH(CustomBeatmapLevel_ctor, &CustomBeatmapLevel::_ctor, void, CustomBeatmapLevel* self, CustomPreviewBeatmapLevel* customPreviewBeatmapLevel) {
    CustomBeatmapLevel_ctor(self, customPreviewBeatmapLevel);
    self->_songDuration_k__BackingField = customPreviewBeatmapLevel->songDuration;
}
