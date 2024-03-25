#include "SongLoader/RuntimeSongLoader.hpp"
#include "hooking.hpp"
#include "logging.hpp"
#include "tasks.hpp"

#include "CustomJSONData.hpp"
#include "SongCore.hpp"

#include "UnityEngine/Object.hpp"
#include "System/Version.hpp"
#include "GlobalNamespace/BeatmapSaveDataHelpers.hpp"
#include "GlobalNamespace/SinglePlayerLevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"

#include "GlobalNamespace/OculusPlatformAdditionalContentModel.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/BeatmapLevelLoader.hpp"
#include "GlobalNamespace/BeatmapLevelDataLoader.hpp"
#include "GlobalNamespace/FileHelpers.hpp"
#include "SongLoader/CustomBeatmapLevelsRepository.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Collections/Generic/IReadOnlyCollection_1.hpp"
#include "System/Collections/Generic/IReadOnlyList_1.hpp"
#include "UnityEngine/Networking/UnityWebRequest.hpp"

#include <regex>

// if the version was still null, override!
MAKE_AUTO_HOOK_MATCH(VersionSerializedData_get_v, &GlobalNamespace::BeatmapSaveDataHelpers::VersionSerializedData::get_v, StringW, GlobalNamespace::BeatmapSaveDataHelpers::VersionSerializedData* self) {
    auto result = VersionSerializedData_get_v(self);
    if (result) return result;
    return GlobalNamespace::BeatmapSaveDataHelpers::getStaticF_noVersion()->ToString();
}

// version getting override implementation
System::Version* GetVersion(StringW data) {
    if (!data) return GlobalNamespace::BeatmapSaveDataHelpers::getStaticF_noVersion();

    auto truncatedText = static_cast<std::string>(data).substr(0, 50);
    static const std::regex versionRegex (R"("_?version"\s*:\s*"[0-9]+\.[0-9]+\.?[0-9]?")", std::regex_constants::optimize);
    std::smatch matches;
    if(std::regex_search(truncatedText, matches, versionRegex)) {
        if(!matches.empty()) {
            auto version = matches[0].str();
            version = version.substr(0, version.length()-1);
            version = version.substr(version.find_last_of('\"')+1, version.length());
            try {
                return System::Version::New_ctor(version);
            } catch(const std::runtime_error& e) {
                ERROR("BeatmapSaveDataHelpers_GetVersion Invalid version: '{}'!", version);
            }
        }
    }

    return GlobalNamespace::BeatmapSaveDataHelpers::getStaticF_noVersion();
}

// version getting override
MAKE_AUTO_HOOK_ORIG_MATCH(BeatmapSaveDataHelpers_GetVersionAsync, &GlobalNamespace::BeatmapSaveDataHelpers::GetVersionAsync, System::Threading::Tasks::Task_1<System::Version*>*, StringW data) {
    return SongCore::StartTask<System::Version*>(std::bind(GetVersion, data));
}

// version getting override
MAKE_AUTO_HOOK_ORIG_MATCH(BeatmapSaveDataHelpers_GetVersion, &GlobalNamespace::BeatmapSaveDataHelpers::GetVersion, System::Version*, StringW data) {
    return GetVersion(data);
}

// create a custom level save data
MAKE_AUTO_HOOK_MATCH(StandardLevelInfoSaveData_DeserializeFromJSONString, &GlobalNamespace::StandardLevelInfoSaveData::DeserializeFromJSONString, GlobalNamespace::StandardLevelInfoSaveData*, StringW stringData) {
    SafePtr<GlobalNamespace::StandardLevelInfoSaveData> original = StandardLevelInfoSaveData_DeserializeFromJSONString(stringData);

    if (!original || !original.ptr()) {
        WARNING("Orig call did not produce valid savedata!");
        return nullptr;
    }

    auto customBeatmapSets = ArrayW<GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmapSet*>(il2cpp_array_size_t(original->difficultyBeatmapSets.size()));

    SongCore::CustomJSONData::CustomLevelInfoSaveData *customSaveData =
            SongCore::CustomJSONData::CustomLevelInfoSaveData::New_ctor(
                original->songName,
                original->songSubName,
                original->songAuthorName,
                original->levelAuthorName,
                original->beatsPerMinute,
                original->songTimeOffset,
                original->shuffle,
                original->shufflePeriod,
                original->previewStartTime,
                original->previewDuration,
                original->songFilename,
                original->coverImageFilename,
                original->environmentName,
                original->allDirectionsEnvironmentName,
                original->environmentNames,
                original->colorSchemes,
                customBeatmapSets
            );

    std::u16string str(stringData ? stringData : u"{}");

    auto sharedDoc = std::make_shared<SongCore::CustomJSONData::DocumentUTF16>();
    customSaveData->doc = sharedDoc;

    rapidjson::GenericDocument<rapidjson::UTF16<char16_t>> &doc = *sharedDoc;
    doc.Parse(str.c_str());

    auto dataItr = doc.FindMember(u"_customData");
    if (dataItr != doc.MemberEnd()) {
        customSaveData->customData = dataItr->value;
    }

    SongCore::CustomJSONData::ValueUTF16 const& beatmapSetsArr = doc.FindMember(u"_difficultyBeatmapSets")->value;

    for (rapidjson::SizeType i = 0; i < beatmapSetsArr.Size(); i++) {
        SongCore::CustomJSONData::ValueUTF16 const& beatmapSetJson = beatmapSetsArr[i];

        auto originalBeatmapSet = original->difficultyBeatmapSets[i];
        auto customBeatmaps = ArrayW<GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmap *>(originalBeatmapSet->difficultyBeatmaps.size());

        auto const& difficultyBeatmaps = beatmapSetJson.FindMember(u"_difficultyBeatmaps")->value;

        for (rapidjson::SizeType j = 0; j < originalBeatmapSet->difficultyBeatmaps.size(); j++) {
            SongCore::CustomJSONData::ValueUTF16 const& difficultyBeatmapJson = difficultyBeatmaps[j];
            auto originalBeatmap = originalBeatmapSet->difficultyBeatmaps[j];

            auto customBeatmap =
                SongCore::CustomJSONData::CustomDifficultyBeatmap::New_ctor(
                    originalBeatmap->difficulty,
                    originalBeatmap->difficultyRank,
                    originalBeatmap->beatmapFilename,
                    originalBeatmap->noteJumpMovementSpeed,
                    originalBeatmap->noteJumpStartBeatOffset,
                    originalBeatmap->beatmapColorSchemeIdx,
                    originalBeatmap->environmentNameIdx
                );

            auto customDataItr = difficultyBeatmapJson.FindMember(u"_customData");
            if (customDataItr != difficultyBeatmapJson.MemberEnd()) {
                customBeatmap->customData = customDataItr->value;
            }

            customBeatmaps[j] = customBeatmap;
        }

        auto customBeatmapSet = SongCore::CustomJSONData::CustomDifficultyBeatmapSet::New_ctor(
            originalBeatmapSet->beatmapCharacteristicName,
            customBeatmaps
        );

        auto customDataItr = beatmapSetJson.FindMember(u"_customData");
        if (customDataItr != beatmapSetJson.MemberEnd()) {
            customBeatmapSet->customData = customDataItr->value;
        }

        customBeatmapSets[i] = customBeatmapSet;
    }
    return customSaveData;
}

// custom songs tab is disabled by default on quest, reenable
MAKE_AUTO_HOOK_ORIG_MATCH(SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels, &GlobalNamespace::SinglePlayerLevelSelectionFlowCoordinator::get_enableCustomLevels, bool, GlobalNamespace::SinglePlayerLevelSelectionFlowCoordinator* self) {
    DEBUG("SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels override returning true");
    return true;
}

using namespace GlobalNamespace;
using namespace System::Threading;
using namespace System::Threading::Tasks;
using namespace System::Collections::Generic;

// we return our own levels repository to which we can add packs we please
MAKE_AUTO_HOOK_ORIG_MATCH(BeatmapLevelsModel_CreateAllLoadedBeatmapLevelPacks, &BeatmapLevelsModel::CreateAllLoadedBeatmapLevelPacks, BeatmapLevelsRepository*, BeatmapLevelsModel* self) {
    auto result = BeatmapLevelsModel_CreateAllLoadedBeatmapLevelPacks(self);

    auto custom = SongCore::SongLoader::CustomBeatmapLevelsRepository::New_ctor();
    auto levelPacks = result->beatmapLevelPacks;
    auto packCount = levelPacks->i___System__Collections__Generic__IReadOnlyCollection_1_T_()->Count;
    for (int i = 0; i < packCount; i++) {
        custom->AddLevelPack(levelPacks->get_Item(i));
    }

    custom->FixBackingDictionaries();

    return custom;
}

// hook to ensure a beatmaps data is actually fully unloaded
MAKE_AUTO_HOOK_MATCH(BeatmapLevelLoader_HandleItemWillBeRemovedFromCache, &BeatmapLevelLoader::HandleItemWillBeRemovedFromCache, void, BeatmapLevelLoader* self, StringW beatmapLevelId, IBeatmapLevelData* beatmapLevel) {
    DEBUG("BeatmapLevelLoader_HandleItemWillBeRemovedFromCache");
    BeatmapLevelLoader_HandleItemWillBeRemovedFromCache(self, beatmapLevelId, beatmapLevel);

    if (beatmapLevelId.starts_with(u"custom_level_"))
        self->_beatmapLevelDataLoader->TryUnload(beatmapLevelId);
}

// override entitelement for custom levels
MAKE_AUTO_HOOK_ORIG_MATCH(OculusPlatformAdditionalContentModel_GetLevelEntitlementStatusInternalAsync, &OculusPlatformAdditionalContentModel::GetLevelEntitlementStatusInternalAsync, Task_1<EntitlementStatus>*, OculusPlatformAdditionalContentModel* self, StringW levelId, CancellationToken cancellationToken) {
    if(levelId.starts_with("custom_level_"))
        return Task_1<EntitlementStatus>::FromResult(EntitlementStatus::Owned);
    return OculusPlatformAdditionalContentModel_GetLevelEntitlementStatusInternalAsync(self, levelId, cancellationToken);
}

// override entitelement for custom level packs
MAKE_AUTO_HOOK_ORIG_MATCH(OculusPlatformAdditionalContentModel_GetPackEntitlementStatusInternalAsync, &OculusPlatformAdditionalContentModel::GetPackEntitlementStatusInternalAsync, Task_1<EntitlementStatus>*, OculusPlatformAdditionalContentModel* self, StringW levelPackId, CancellationToken cancellationToken) {
    if(levelPackId.starts_with("custom_levelPack_"))
        return Task_1<EntitlementStatus>::FromResult(EntitlementStatus::Owned);
    return OculusPlatformAdditionalContentModel_GetPackEntitlementStatusInternalAsync(self, levelPackId, cancellationToken);
}

MAKE_AUTO_HOOK_ORIG_MATCH(BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, &BeatmapLevelsModel::ReloadCustomLevelPackCollectionAsync, Task_1<GlobalNamespace::BeatmapLevelsRepository*>*, BeatmapLevelsModel* self, CancellationToken cancellationToken) {
    // if songs are loaded and not refreshing, return the repo with fromresult
    if (SongCore::API::Loading::AreSongsLoaded()) {
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

// reading files sucks for file paths
MAKE_AUTO_HOOK_ORIG_MATCH(FileHelpers_GetEscapedURLForFilePath, &FileHelpers::GetEscapedURLForFilePath, StringW, StringW filePath) {
    std::u16string str = filePath;
    int index = str.find_last_of('/') + 1;
    StringW dir = str.substr(0, index);
    StringW fileName = UnityEngine::Networking::UnityWebRequest::EscapeURL(str.substr(index, str.size()));
    std::replace(fileName.begin(), fileName.end(), u'+', u' '); // '+' breaks stuff even though it's supposed to be valid encoding ¯\_(ツ)_/¯
    return u"file://" + dir + fileName;
}

// get the level data async
MAKE_AUTO_HOOK_ORIG_MATCH(BeatmapLevelsModel_LoadBeatmapLevelDataAsync, &BeatmapLevelsModel::LoadBeatmapLevelDataAsync, Task_1<LoadBeatmapLevelDataResult>*, BeatmapLevelsModel* self, StringW levelID, CancellationToken token) {
    if (levelID.starts_with(u"custom_level_")) {
        return SongCore::StartTask<LoadBeatmapLevelDataResult>([=](SongCore::CancellationToken token){
            static auto Error = LoadBeatmapLevelDataResult(true, nullptr);
            auto level = SongCore::API::Loading::GetLevelByLevelID(static_cast<std::string>(levelID));
            if (!level || token.IsCancellationRequested) return Error;
            auto data = level->beatmapLevelData;
            if (!data) return Error;
            return LoadBeatmapLevelDataResult::Success(data);
        }, std::forward<SongCore::CancellationToken>(token));
    }

    return BeatmapLevelsModel_LoadBeatmapLevelDataAsync(self, levelID, token);
}

// get the level data async
MAKE_AUTO_HOOK_ORIG_MATCH(BeatmapLevelsModel_CheckBeatmapLevelDataExistsAsync, &BeatmapLevelsModel::CheckBeatmapLevelDataExistsAsync, Task_1<bool>*, BeatmapLevelsModel* self, StringW levelID, CancellationToken token) {
    if (levelID.starts_with(u"custom_level_")) {
        return SongCore::StartTask<bool>([=](SongCore::CancellationToken token){
            auto level = SongCore::API::Loading::GetLevelByLevelID(static_cast<std::string>(levelID));
            if (!level) return false;
            return level->beatmapLevelData != nullptr;
        }, std::forward<SongCore::CancellationToken>(token));
    }

    return BeatmapLevelsModel_CheckBeatmapLevelDataExistsAsync(self, levelID, token);
}

// override getting beatmap level if original method didn't return anything
MAKE_AUTO_HOOK_MATCH(BeatmapLevelsModel_GetBeatmapLevel, &BeatmapLevelsModel::GetBeatmapLevel, BeatmapLevel*, BeatmapLevelsModel* self, StringW levelID) {
    auto result = BeatmapLevelsModel_GetBeatmapLevel(self, levelID);
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
MAKE_AUTO_HOOK_ORIG_MATCH(
    PlatformLeaderboardViewController_Refresh,
    &PlatformLeaderboardViewController::Refresh,
    void,
    PlatformLeaderboardViewController* self,
    bool showLoadingIndicator,
    bool clear
) {
    auto levelId = self->_beatmapKey.levelId;
    if (levelId.starts_with(u"custom_level_"))
        self->_loadingControl->ShowText(BGLib::Polyglot::Localization::Get("CUSTOM_LEVELS_LEADERBOARDS_NOT_SUPPORTED"), false);
    else
        PlatformLeaderboardViewController_Refresh(self, showLoadingIndicator, clear);
}
