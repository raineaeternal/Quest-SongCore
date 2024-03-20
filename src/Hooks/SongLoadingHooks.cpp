#include "hooking.hpp"
#include "logging.hpp"

#include "CustomJSONData.hpp"
#include "UnityEngine/Object.hpp"
#include "System/Version.hpp"
#include "GlobalNamespace/BeatmapSaveDataHelpers.hpp"
#include "GlobalNamespace/SinglePlayerLevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"

#include "GlobalNamespace/AdditionalContentModel.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/FileHelpers.hpp"
#include "SongLoader/CustomBeatmapLevelsRepository.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "UnityEngine/Networking/UnityWebRequest.hpp"

#include <regex>

MAKE_AUTO_HOOK_MATCH(StandardLevelInfoSaveData_DeserializeFromJSONString, &GlobalNamespace::StandardLevelInfoSaveData::DeserializeFromJSONString, GlobalNamespace::StandardLevelInfoSaveData*, StringW stringData) {
    DEBUG("StandardLevelInfoSaveData_DeserializeFromJSONString");

    SafePtr<GlobalNamespace::StandardLevelInfoSaveData> original = StandardLevelInfoSaveData_DeserializeFromJSONString(stringData);

    if (!original || !original.ptr()) {
        DEBUG("Orig call did not produce valid savedata!");
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
    for (auto pack : ArrayW<GlobalNamespace::BeatmapLevelPack*>(result->beatmapLevelPacks)) {
        custom->AddLevelPack(pack);
    }

    return custom;
}

MAKE_AUTO_HOOK_ORIG_MATCH(AdditionalContentModel_GetLevelEntitlementStatusAsync, &AdditionalContentModel::GetLevelEntitlementStatusInternalAsync, Task_1<EntitlementStatus>*, AdditionalContentModel* self, StringW levelId, CancellationToken cancellationToken) {
    if(levelId.starts_with("custom_level_"))
        return Task_1<EntitlementStatus>::FromResult(EntitlementStatus::Owned);
    return AdditionalContentModel_GetLevelEntitlementStatusAsync(self, levelId, cancellationToken);
}

MAKE_AUTO_HOOK_ORIG_MATCH(AdditionalContentModel_GetPackEntitlementStatusAsync, &AdditionalContentModel::GetPackEntitlementStatusInternalAsync, Task_1<EntitlementStatus>*, AdditionalContentModel* self, StringW levelPackId, CancellationToken cancellationToken) {
    if(levelPackId.starts_with("custom_levelPack_"))
        return Task_1<EntitlementStatus>::FromResult(EntitlementStatus::Owned);
    return AdditionalContentModel_GetPackEntitlementStatusAsync(self, levelPackId, cancellationToken);
}

MAKE_AUTO_HOOK_ORIG_MATCH(BeatmapLevelsModel_ReloadCustomLevelPackCollectionAsync, &BeatmapLevelsModel::ReloadCustomLevelPackCollectionAsync, Task_1<GlobalNamespace::BeatmapLevelsRepository*>*, BeatmapLevelsModel* self, CancellationToken cancellationToken) {
    return Task_1<GlobalNamespace::BeatmapLevelsRepository*>::FromResult(self->_allLoadedBeatmapLevelsRepository);
}

MAKE_AUTO_HOOK_ORIG_MATCH(FileHelpers_GetEscapedURLForFilePath, &FileHelpers::GetEscapedURLForFilePath, StringW, StringW filePath) {
    std::u16string str = filePath;
    int index = str.find_last_of('/') + 1;
    StringW dir = str.substr(0, index);
    StringW fileName = UnityEngine::Networking::UnityWebRequest::EscapeURL(str.substr(index, str.size()));
    std::replace(fileName.begin(), fileName.end(), u'+', u' '); // '+' breaks stuff even though it's supposed to be valid encoding ¯\_(ツ)_/¯
    return u"file://" + dir + fileName;
}
