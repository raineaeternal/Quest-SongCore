#include "SongLoader/RuntimeSongLoader.hpp"
#include "hooking.hpp"
#include "logging.hpp"
#include "tasks.hpp"

#include "CustomJSONData.hpp"
#include "SongCore.hpp"
#include "SongLoader/CustomBeatmapLevelsRepository.hpp"

#include "GlobalNamespace/BeatmapSaveDataHelpers.hpp"
#include "GlobalNamespace/SinglePlayerLevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/LevelSearchViewController.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/PlatformLeaderboardViewController.hpp"
#include "GlobalNamespace/LoadingControl.hpp"
#include "GlobalNamespace/OculusPlatformAdditionalContentModel.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/BeatmapLevelsEntitlementModel.hpp"
#include "GlobalNamespace/BeatmapLevelLoader.hpp"
#include "GlobalNamespace/BeatmapLevelDataLoader.hpp"
#include "GlobalNamespace/FileHelpers.hpp"
#include "GlobalNamespace/GameScenesManager.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "UnityEngine/AudioSettings.hpp"
#include "GlobalNamespace/LevelScenesTransitionSetupData.hpp"
#include "GlobalNamespace/ReferenceCountingCache_2.hpp"
#include "GlobalNamespace/AudioClipAsyncLoaderExtensions.hpp"
#include <unordered_map>

#include "custom-types/shared/coroutine.hpp"

#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Threading/Tasks/Task.hpp"
#include "System/Collections/Generic/IReadOnlyCollection_1.hpp"
#include "System/Collections/Generic/IReadOnlyList_1.hpp"
#include "System/Version.hpp"
#include "System/GC.hpp"

#include "UnityEngine/AudioConfiguration.hpp"
#include "UnityEngine/AudioSettings.hpp"
#include "UnityEngine/Networking/UnityWebRequest.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Sprite.hpp"

#include "BGLib/Polyglot/Localization.hpp"
#include "BGLib/DotnetExtension/Collections/LRUCache_2.hpp"
#include "SongLoader/CustomBeatmapLevel.hpp"

#include "utf8.h"
#include <string>
#include "Utils/SaveDataVersion.hpp"

// custom songs tab is disabled by default on quest, reenable
MAKE_AUTO_ORIG_HOOK_MATCH(SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels, &GlobalNamespace::SinglePlayerLevelSelectionFlowCoordinator::get_enableCustomLevels, bool, GlobalNamespace::SinglePlayerLevelSelectionFlowCoordinator* self) {
    DEBUG("SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels override returning true");
    return true;
}

using namespace GlobalNamespace;
using namespace System::Threading;
using namespace System::Threading::Tasks;
using namespace System::Collections::Generic;

namespace {
using SongAudioTask = Task_1<UnityW<UnityEngine::AudioClip>>;
// Entries are erased with the level LRU. The LRU/cache owns the managed objects;
// this table records identity only and never extends their lifetime.
// A loader-cache hit may belong only to a menu request. Track this level's own
// acquisition so revisits reuse its reference and level eviction releases it once.
std::unordered_map<IBeatmapLevelData*, SongAudioTask*> levelAudioRequests;
thread_local GameplayCoreSceneSetupData* gameplayAudioContext = nullptr;
struct GameplayAudioScope {
    GameplayCoreSceneSetupData* previous;
    explicit GameplayAudioScope(GameplayCoreSceneSetupData* current) : previous(gameplayAudioContext) { gameplayAudioContext = current; }
    ~GameplayAudioScope() { gameplayAudioContext = previous; }
};
}

// Let LoadSong identify the gameplay setup during this async resume.
// Restore the previous context when MoveNext returns.
MAKE_AUTO_ORIG_HOOK_MATCH(SongAudio_GameplayLoadMoveNext, &GameplayCoreSceneSetupData__LoadAndTransformBeatmapDataAsync_d__32::MoveNext,
    void, GameplayCoreSceneSetupData__LoadAndTransformBeatmapDataAsync_d__32* self) {
    GameplayAudioScope scope(self->__4__this);
    SongAudio_GameplayLoadMoveNext(self);
}

MAKE_AUTO_ORIG_HOOK_MATCH(SongAudio_LoadSong, &AudioClipAsyncLoaderExtensions::LoadSong,
    SongAudioTask*, AudioClipAsyncLoader* loader, IBeatmapLevelData* data) {
    auto context = gameplayAudioContext;

    auto level = context ? context->beatmapLevel : nullptr;
    auto levelId = level ? level->levelID : StringW(nullptr);

    auto model = context ? context->_beatmapLevelsModel : nullptr;
    auto levelLoader = model ? i2c::try_cast<BeatmapLevelLoader*>(reinterpret_cast<System::Object*>(model->levelLoader)) : nullptr;
    auto levelCache = levelLoader ? levelLoader->_loadedBeatmapLevelDataCache : nullptr;

    // Do we have cached gameplay data for this level? If so, we can skip the normal async load and return the cached task.
    IBeatmapLevelData* cachedData = nullptr;
    bool cachedGameplay = context && loader == context->_audioClipAsyncLoader && data == context->get_beatmapLevelData()
        && levelId && levelId.starts_with(u"custom_level_") && levelCache
        && levelCache->TryGetFromCache(levelId, by_ref(cachedData)) && cachedData == data;
    if (!cachedGameplay) return SongAudio_LoadSong(loader, data);
    // The state machine performs one song acquisition per resume. Nested/menu
    // work after that acquisition must keep the original reference-count behavior.
    gameplayAudioContext = nullptr;
    
    // Check if the level data is a FileSystemBeatmapLevelData, which is the type used for custom levels. 
    // If it is, we can use the songAudioClipPath to get the cache key and check if we have a cached audio task for this level data.
    auto fileData = i2c::try_cast<FileSystemBeatmapLevelData*>(reinterpret_cast<System::Object*>(data));
    if (!fileData) return SongAudio_LoadSong(loader, data);
    auto key = loader->GetCacheKey(fileData->songAudioClipPath);
    auto cache = reinterpret_cast<ReferenceCountingCache_2<int32_t, SongAudioTask*>*>(loader->_cache);
    
    // Find existing task for this level data in the levelAudioRequests map.
    // If we have a cached task and it's still valid, return it. Otherwise, load the song and store the new task in the map.
    SongAudioTask* task = nullptr;
    auto owned = levelAudioRequests.find(data);
    if (owned != levelAudioRequests.end() && cache->TryGet(key, by_ref(task))
        && task == owned->second && cache->GetReferenceCount(key) > 0) {
        return task;
    }
    // We don't have a valid cached task, so we need to load the song and store the new task in the map.
    task = SongAudio_LoadSong(loader, data);
    if (task) levelAudioRequests[data] = task;
    return task;
}

// If the garbage collector is enabled then the game resets the audio configuration, which invalidates the custom level AudioClip
// reimplement without the audio configuration change and instead reset audio configuration
// on LevelScenesTransitionSetupDataSO::BeforeScenesWillBeActivatedAsync
MAKE_AUTO_HOOK_MATCH(
    GameScenesManager_ScenesTransitionCoroutine,
    &GameScenesManager::ScenesTransitionCoroutine,
    System::Collections::IEnumerator *, GameScenesManager *self,
    ::GlobalNamespace::ScenesTransitionSetupData *newScenesTransitionSetupData,
    ::System::Collections::Generic::IReadOnlyList_1<::StringW> *scenesToPresent,
    ::GlobalNamespace::GameScenesManager_ScenePresentType presentType,
    ::System::Collections::Generic::IReadOnlyList_1<::StringW> *scenesToDismiss,
    ::GlobalNamespace::GameScenesManager_SceneDismissType dismissType,
    float_t minDuration, bool canTriggerGarbageCollector,
    ::System::Action *afterMinDurationCallback,
    ::System::Action_1<::Zenject::DiContainer *> *extraBindingsCallback,
    ::System::Action_1<::Zenject::DiContainer *> *finishCallback) {
//   resetAudio = false;
  return GameScenesManager_ScenesTransitionCoroutine(
      self, newScenesTransitionSetupData, scenesToPresent, presentType,
      scenesToDismiss, dismissType, minDuration, canTriggerGarbageCollector,
      afterMinDurationCallback, extraBindingsCallback, finishCallback);
}
static void AudioSettings_Reset() {
  static i2c::result<function_ptr_t<::UnityEngine::AudioConfiguration>>
      UnityEngine_AudioSettings_GetConfiguration =
          i2c::resolve_icall <
          UnityEngine::AudioConfiguration>("AudioSettings::GetConfiguration");
  static i2c::result<function_ptr_t<bool, ::UnityEngine::AudioConfiguration>> UnityEngine_AudioSettings_SetConfiguration =
      i2c::resolve_icall<bool, ::UnityEngine::AudioConfiguration>(
          "AudioSettings::SetConfiguration");
  //   UnityEngine::AudioSettings::Reset(
  //       UnityEngine::AudioSettings::GetConfiguration());

  if (!UnityEngine_AudioSettings_GetConfiguration || !UnityEngine_AudioSettings_SetConfiguration) {
    ERROR("Failed to resolve AudioSettings::GetConfiguration or AudioSettings::SetConfiguration");
    return;
  }

  auto config = UnityEngine_AudioSettings_GetConfiguration.value()();
  UnityEngine_AudioSettings_SetConfiguration.value()(config);
}

// Reset AudioSettings
// This is to ensure audio is intact between scenes, and custom audio gets loaded after audio settings is reset.
// Without this patch, one can get the game to have no audio after restarting the map several times.
MAKE_AUTO_HOOK_MATCH(
    LevelScenesTransitionSetupDataSO_BeforeScenesWillBeActivatedAsync,
    &LevelScenesTransitionSetupData::BeforeScenesWillBeActivatedAsync,
    ::System::Threading::Tasks::Task *, LevelScenesTransitionSetupData *self) {
  auto sceneSetupData = self->gameplayCoreSceneSetupData;
  AudioClipAsyncLoader *audioClipLoader = sceneSetupData->_audioClipAsyncLoader;

  // Clear cache of AudioClip for current loading beatmap, only if it is a
  // custom level.
  if (auto levelID = sceneSetupData->beatmapLevel->levelID;
      levelID.starts_with(u"custom_level")) {
    // Get path of current loading Beatmap
    const auto customLevel = SongCore::API::Loading::GetLevelByLevelID(
        static_cast<std::string>(levelID));
    auto songAudioClipPath =
        ((FileSystemBeatmapLevelData *)customLevel->beatmapLevelData)
            ->songAudioClipPath;

    // Fetch Beatmap AudioClip cache key.
    auto cacheKey = audioClipLoader->GetCacheKey(songAudioClipPath);

    // Remove current Beatmap AudioClip from Cache to load a new one.
    auto audioClipCache =
        (ReferenceCountingCache_2<
            int32_t, Task_1<::UnityW<::UnityEngine::AudioClip>> *> *)
            audioClipLoader->_cache;
    // Releases use the key, so replacing owned audio lets an old release destroy
    // the new gameplay clip. Keep the entry and skip its reset while owned.
    if (audioClipCache->GetReferenceCount(cacheKey) > 0) {
      return LevelScenesTransitionSetupDataSO_BeforeScenesWillBeActivatedAsync(self);
    }

    // TODO: Call unstripped libunity. Reset/reload only an unowned current-song entry.
    AudioSettings_Reset();
    // A synchronous reset callback may acquire the song. Recheck before removal.
    if (audioClipCache->GetReferenceCount(cacheKey) > 0) {
      return LevelScenesTransitionSetupDataSO_BeforeScenesWillBeActivatedAsync(self);
    }
    audioClipCache->_referencesCount->Remove(cacheKey);
    audioClipCache->_items->Remove(cacheKey);
  } else {
    AudioSettings_Reset();
  }

  return LevelScenesTransitionSetupDataSO_BeforeScenesWillBeActivatedAsync(
      self);
}

// we return our own levels repository to which we can add packs we please
MAKE_AUTO_ORIG_HOOK_MATCH(BeatmapLevelsModel_CreateAllLoadedBeatmapLevelPacks, &BeatmapLevelsModel::LoadAllBeatmapLevelPacks, void, BeatmapLevelsModel* self) {
    BeatmapLevelsModel_CreateAllLoadedBeatmapLevelPacks(self);

    auto custom = SongCore::SongLoader::CustomBeatmapLevelsRepository::New_ctor();
    auto levelPacks = self->____allLoadedBeatmapLevelsRepository->____beatmapLevelPacks;
    auto packCount = levelPacks.size();
    for (int i = 0; i < packCount; i++) {
        custom->AddLevelPack(levelPacks[i]);
    }

    custom->FixBackingDictionaries();

    self->_allLoadedBeatmapLevelsRepository = custom;
}

// hook to ensure a beatmaps data is actually fully unloaded
MAKE_AUTO_HOOK_MATCH(BeatmapLevelLoader_HandleItemWillBeRemovedFromCache, &BeatmapLevelLoader::HandleItemWillBeRemovedFromCache, void, BeatmapLevelLoader* self, StringW beatmapLevelId, IBeatmapLevelData* beatmapLevel) {
    DEBUG("BeatmapLevelLoader_HandleItemWillBeRemovedFromCache");
    // Track the item being removed from the cache so we can remove it from our own cache as well
    levelAudioRequests.erase(beatmapLevel);
    BeatmapLevelLoader_HandleItemWillBeRemovedFromCache(self, beatmapLevelId, beatmapLevel);

    if (beatmapLevelId.starts_with(u"custom_level_"))
        self->_beatmapLevelDataLoader->TryUnload(beatmapLevelId);
}

// ifd out on quest, just check for custom_level_ prepend and say owned if so
MAKE_AUTO_ORIG_HOOK_MATCH(BeatmapLevelsEntitlementModel_GetLevelEntitlementStatusAsync, &BeatmapLevelsEntitlementModel::GetLevelEntitlementStatusAsync, Task_1<EntitlementStatus>*, BeatmapLevelsEntitlementModel* self, StringW levelID, CancellationToken token) {
    if (levelID.starts_with(u"custom_level_")) {
        return Task_1<EntitlementStatus>::FromResult(EntitlementStatus::Owned);
    }
    return BeatmapLevelsEntitlementModel_GetLevelEntitlementStatusAsync(self, levelID, token);
}

// ifd out on quest, just check for custom_levelPack_ prepend and say owned if so
MAKE_AUTO_ORIG_HOOK_MATCH(BeatmapLevelsEntitlementModel_GetPackEntitlementStatusAsync, &BeatmapLevelsEntitlementModel::GetPackEntitlementStatusAsync, Task_1<EntitlementStatus>*, BeatmapLevelsEntitlementModel* self, StringW levelPackID, CancellationToken token) {
    if (levelPackID.starts_with(u"custom_levelPack_")) {
        return Task_1<EntitlementStatus>::FromResult(EntitlementStatus::Owned);
    }
    return BeatmapLevelsEntitlementModel_GetPackEntitlementStatusAsync(self, levelPackID, token);
}

MAKE_AUTO_ORIG_HOOK_MATCH(BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, &BeatmapLevelsModel::ReloadCustomLevelPackCollectionAsync, Task_1<GlobalNamespace::BeatmapLevelsRepository*>*, BeatmapLevelsModel* self, CancellationToken cancellationToken) {
    // if songs are loaded and not refreshing, return the repo with fromresult
    if (SongCore::API::Loading::AreSongsLoaded() && !SongCore::API::Loading::AreSongsRefreshing()) {
        return Task_1<GlobalNamespace::BeatmapLevelsRepository*>::FromResult(static_cast<GlobalNamespace::BeatmapLevelsRepository*>(SongCore::API::Loading::GetCustomBeatmapLevelsRepository()));
    }

    // levels weren't loaded or we are refreshing right now, so make the user wait
    return SongCore::StartTask<GlobalNamespace::BeatmapLevelsRepository*>([](SongCore::CancellationToken cancelToken) -> GlobalNamespace::BeatmapLevelsRepository* {
        using namespace std::chrono_literals;
        auto loader = SongCore::SongLoader::RuntimeSongLoader::get_instance();
        while (loader->AreSongsRefreshing && !cancelToken.IsCancellationRequested) std::this_thread::sleep_for(100ms);
        return loader->CustomBeatmapLevelsRepository;
    }, std::forward<SongCore::CancellationToken>(cancellationToken));
}

// gets the char16 representation of the 2 nibbles that fit 1 char
std::pair<char16_t, char16_t> getByteChars(char c) {
    static char16_t nibbleToChar[] = u"0123456789abcdef";
    auto lower = c & 0b1111;
    auto upper = (c >> 4) & 0b1111;

    return {nibbleToChar[lower], nibbleToChar[upper]};
}

// manually implement escaping the filepath url. as it turns out most of the code the game does is good except it escapes / as well which breaks filepaths, so we just escape with a limited charset
std::u16string escape(std::u16string_view url) {
    static char16_t forbidden[] = u"@&;:<>=?\"'\\!#%+$,{}|^[]`";
    static auto forbiddenEnd = forbidden + (sizeof(forbidden) / sizeof(char16_t));
    static auto isForbidden = [](char16_t c) { return std::find(forbidden, forbiddenEnd, c) != forbiddenEnd; };

    // i'd have preferred to use wstringstream but it doesn't convert to u16 as easily
    std::u16string escaped;
    escaped.reserve(url.size());

    for (auto c : url) {
        if (isForbidden(c)) {
            escaped.push_back(u'%');
            // we lose width here but all forbidden chars are only as big as 1 byte (char) anyway
            auto [lc, uc] = getByteChars(static_cast<char>(c));
            escaped.push_back(uc);
            escaped.push_back(lc);
        } else {
            escaped.push_back(c);
        }
    }

    return escaped;
}

// reading files sucks for file paths
MAKE_AUTO_ORIG_HOOK_MATCH(FileHelpers_GetEscapedURLForFilePath, &FileHelpers::GetEscapedURLForFilePath, StringW, StringW filePath) {
    std::u16string_view view(filePath);
    if (view.find(u"://") != std::string::npos) { // check if it's already a URL
        return filePath;
    }
    return fmt::format("file://{}", utf8::utf16to8(escape(filePath)));
}

// Custom data is already parsed by SongCore. Register it in the game's bounded
// level cache so eviction calls UnloadSong and balances the gameplay audio load.
// Keep LRU access on the calling thread instead of a StartTask worker.
MAKE_AUTO_ORIG_HOOK_MATCH(BeatmapLevelsModel_LoadBeatmapLevelDataAsync, &BeatmapLevelsModel::LoadBeatmapLevelDataAsync, Task_1<LoadBeatmapLevelDataResult>*, BeatmapLevelsModel* self, StringW levelID, GlobalNamespace::BeatmapLevelDataVersion beatmapLevelDataVersion, CancellationToken token) {
    if (levelID.starts_with(u"custom_level_")) {
        auto errorType = System::Nullable_1(true, LoadBeatmapLevelDataResult_ErrorType::BeatmapLevelNotFoundInRepository);
        auto error = LoadBeatmapLevelDataResult(errorType, nullptr);
        if (token.IsCancellationRequested) return Task_1<LoadBeatmapLevelDataResult>::FromResult(error);
        
        // If we have a cached level, return it. Otherwise, get the level from SongCore and add it to the cache.
        auto loader = i2c::try_cast<BeatmapLevelLoader*>(reinterpret_cast<System::Object*>(self->levelLoader));
        auto cache = loader ? loader->_loadedBeatmapLevelDataCache : nullptr;
        IBeatmapLevelData* data = nullptr;
        if (cache && cache->TryGetFromCache(levelID, by_ref(data)) && data) {
            return Task_1<LoadBeatmapLevelDataResult>::FromResult(LoadBeatmapLevelDataResult::Success(data));
        }

        // Get the level from SongCore and add it to the cache if it exists. If it doesn't exist, return an error.
        auto level = SongCore::API::Loading::GetLevelByLevelID(static_cast<std::string>(levelID));
        data = level ? level->beatmapLevelData : nullptr;
        if (!data) return Task_1<LoadBeatmapLevelDataResult>::FromResult(error);
        if (cache) cache->Add(levelID, data);
        else WARNING("Cannot register custom level {} in the game's cache: unsupported level loader",static_cast<std::string>(levelID));
        return Task_1<LoadBeatmapLevelDataResult>::FromResult(LoadBeatmapLevelDataResult::Success(data));
    }
    return BeatmapLevelsModel_LoadBeatmapLevelDataAsync(self, levelID, beatmapLevelDataVersion, token);
}

// get the level data async
// TODO: rip out this level data loading from the SongLoader/LevelLoader.cpp and implement it async here to improve level loading speed and not do redundant things here
MAKE_AUTO_ORIG_HOOK_MATCH(BeatmapLevelsModel_CheckBeatmapLevelDataExistsAsync, &BeatmapLevelsModel::CheckBeatmapLevelDataExistsAsync, Task_1<bool>*, BeatmapLevelsModel* self, StringW levelID, GlobalNamespace::BeatmapLevelDataVersion beatmapLevelDataVersion, CancellationToken token) {
    if (levelID.starts_with(u"custom_level_")) {
        return SongCore::StartTask<bool>([=](SongCore::CancellationToken token){
            auto level = SongCore::API::Loading::GetLevelByLevelID(static_cast<std::string>(levelID));
            if (!level) return false;
            return level->beatmapLevelData != nullptr;
        }, std::forward<SongCore::CancellationToken>(token));
    }

    return BeatmapLevelsModel_CheckBeatmapLevelDataExistsAsync(self, levelID, beatmapLevelDataVersion, token);
}

// override getting beatmap level if original method didn't return anything
MAKE_AUTO_HOOK_MATCH(BeatmapLevelsRepository_GetBeatmapLevelById, &BeatmapLevelsRepository::GetBeatmapLevelById, GlobalNamespace::BeatmapLevel*, BeatmapLevelsRepository* self, ::StringW levelId, bool ignoreCase) {
    auto result = BeatmapLevelsRepository_GetBeatmapLevelById(self, levelId, ignoreCase);
    if (!result && levelId.starts_with(u"custom_level_")) {
        result = SongCore::API::Loading::GetLevelByLevelID(static_cast<std::string>(levelId));
    }
    return result;
}

MAKE_AUTO_HOOK_MATCH(BeatmapLevelsModel_GetBeatmapLevel, &BeatmapLevelsModel::GetBeatmapLevel, BeatmapLevel*, BeatmapLevelsModel* self, StringW levelID, bool ignoreCase) {
    auto result = BeatmapLevelsModel_GetBeatmapLevel(self, levelID, ignoreCase);
    if (!result && levelID.starts_with(u"custom_level_")) {
        result = SongCore::API::Loading::GetLevelByLevelID(static_cast<std::string>(levelID));
    }

    return result;
}

// input array can have duplicates, we change the input for that reason
MAKE_AUTO_HOOK_MATCH(BeatmapLevelPack_CreateLevelPackForFiltering, &BeatmapLevelPack::CreateLevelPackForFiltering, BeatmapLevelPack*, ArrayW<BeatmapLevel*> beatmapLevels) {
    // a set considers 2 elements equivalent if !(a < b) && !(b < a) which for pointers is ideal since 2 same pointers are the same level
    std::set<BeatmapLevel*> levelSet;
    for (auto level : beatmapLevels) levelSet.insert(level);

    ArrayW<BeatmapLevel*> uniqueLevels(levelSet.size());
    for (auto idx = 0; auto level : levelSet) uniqueLevels[idx++] = level;
    INFO("Filtered levels for uniqueness, had an array of size {}, now {}", beatmapLevels.size(), uniqueLevels.size());

    return BeatmapLevelPack_CreateLevelPackForFiltering(uniqueLevels);
}

// fix error message on custom levels, as the game doesn't correctly display the error message here on quest due to an ifd out check
MAKE_AUTO_ORIG_HOOK_MATCH(
    PlatformLeaderboardViewController_Refresh,
    &PlatformLeaderboardViewController::Refresh,
    void,
    PlatformLeaderboardViewController* self,
    bool showLoadingIndicator,
    bool clear
) {
    auto levelId = self->_beatmapKey.levelId;
    if (levelId.starts_with(u"custom_level_")) {
        self->StopAllCoroutines();
        self->ClearContent();
        self->_loadingControl->ShowText(BGLib::Polyglot::Localization::Get("CUSTOM_LEVELS_LEADERBOARDS_NOT_SUPPORTED"), false);
    } else {
        PlatformLeaderboardViewController_Refresh(self, showLoadingIndicator, clear);
    }
}

// keep level search and filters when refreshing
MAKE_AUTO_HOOK_MATCH(
    LevelFilteringNavigationController_UpdateSecondChildControllerContent,
    &LevelFilteringNavigationController::UpdateSecondChildControllerContent,
    void,
    LevelFilteringNavigationController* self,
    SelectLevelCategoryViewController::LevelCategory levelCategory
) {
    std::optional<LevelFilter> searchFilter = std::nullopt;
    bool inSearch = levelCategory == SelectLevelCategoryViewController::LevelCategory::Favorites
                    || levelCategory == SelectLevelCategoryViewController::LevelCategory::All;
    // cancellationTokenSource is only non null during UpdateCustomSongs
    if (inSearch && self->_cancellationTokenSource && self->_levelSearchViewController) {
        searchFilter = self->_levelSearchViewController->_currentSearchFilter;
        if (searchFilter->searchText == nullptr) searchFilter->searchText = "";
    }
    // packs are not initialized, so filters aren't yet either
    if (inSearch && !self->_allBeatmapLevelPacks)
        self->_levelSearchViewController->ResetAllFilterSettings(levelCategory == SelectLevelCategoryViewController::LevelCategory::Favorites);

    LevelFilteringNavigationController_UpdateSecondChildControllerContent(self, levelCategory);

    if (searchFilter) self->_levelSearchViewController->Refresh(by_ref(*searchFilter));
}


// Fix levels using regular name instead of serialized name.
MAKE_AUTO_HOOK_MATCH(
    EnvironmentsListModel_GetEnvironmentInfoBySerializedName,
    &EnvironmentsListModel::GetEnvironmentInfoBySerializedName,
    UnityW<GlobalNamespace::EnvironmentInfoSO>,
    EnvironmentsListModel* self,
    StringW environmentSerializedName
) {
    auto result = EnvironmentsListModel_GetEnvironmentInfoBySerializedName(self, environmentSerializedName);
    //Fix here
    if (!result) {
        //This should be a rare case
        auto envList = ListW<UnityW<GlobalNamespace::EnvironmentInfoSO>>::New();
        envList->AddRange(self->environmentInfos->i___System__Collections__Generic__IEnumerable_1_T_());
        for(auto& env : envList) {
            if (env->environmentName == environmentSerializedName) {
                result = env;
            }
        }
    }

    return result;
}


// https://github.com/Kylemc1413/SongCore/pull/148/files
// This partially fixes a bug that was introduced in v1.36.0, which saves null covers when the loading operation is canceled.
// It still shows the default cover at times, but at least it reloads it now.
// TODO: Remove when fixed.
// Generic Patching ):

/*
using LRUCacheInst = BGLib::DotnetExtension::Collections::LRUCache_2<StringW, UnityEngine::Sprite*>;

MAKE_HOOK(LRUCache_Add, nullptr, void, LRUCacheInst* self, StringW key, UnityEngine::Sprite* value, MethodInfo* methodInfo) {
    if (value != nullptr)
        LRUCache_Add(self, key, value, methodInfo);
}

struct Auto_Hook_LRUCache_Add {
    static void Auto_Hook_LRUCache_Add_Install() {
        static constexpr auto logger = Paper::ConstLoggerContext("SongCore_Install_LRUCache_Add");
        static auto addr = il2cpp_utils::FindMethodUnsafe(classof(LRUCacheInst*), "Add", 2);
        INSTALL_HOOK_DIRECT(logger, LRUCache_Add, (void*) addr->methodPointer);
    }
    Auto_Hook_LRUCache_Add() {
        SongCore::Hooking::AddInstallFunc(Auto_Hook_LRUCache_Add_Install);
    }
};
static Auto_Hook_LRUCache_Add Auto_Hook_Instance_LRUCache_Add;
*/
