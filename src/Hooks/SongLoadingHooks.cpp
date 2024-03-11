#include "hooking.hpp"
#include "logging.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"

#include "shared/CustomJSONData.hpp"
#include "UnityEngine/Object.hpp"
#include "System/Version.hpp"
#include "GlobalNamespace/BeatmapSaveDataHelpers.hpp"
#include "GlobalNamespace/SinglePlayerLevelSelectionFlowCoordinator.hpp"

#include <regex>

MAKE_AUTO_HOOK_MATCH(BeatmapSaveDataHelpers_GetVersion, &GlobalNamespace::BeatmapSaveDataHelpers::GetVersion, System::Version*, StringW data) {
    DEBUG("BeatmapSaveDataHelpers_GetVersion");
    auto truncatedText = data.operator std::string().substr(0, 50);
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
                INFO("BeatmapSaveDataHelpers_GetVersion Invalid version: \"%s\"!", version.c_str());
            }
        }
    }
    return System::Version::New_ctor("2.0.0");
}

MAKE_AUTO_HOOK_MATCH(StandardLevelInfoSaveData_DeserializeFromJSONString, &GlobalNamespace::StandardLevelInfoSaveData::DeserializeFromJSONString, GlobalNamespace::StandardLevelInfoSaveData *, StringW stringData) {
    DEBUG("StandardLevelInfoSaveData_DeserializeFromJSONString");

    SafePtr<GlobalNamespace::StandardLevelInfoSaveData> original;

    // replacing the 2.x.x version with 2.0.0 is assumed safe because
    // minor/patch versions should NOT break the schema and therefore deemed
    // readable by RSL even if the new fields are not parsed
    // short circuit
    // 1.0.0 and 2.0.0 are supported by basegame through string equality
    // checks if (!stringData->Contains("1.0.0") &&
    // !stringData->Contains("2.0.0")) { https://regex101.com/r/jJAvvE/3
    // Verified result: https://godbolt.org/z/MvfW3eh7q
    // Checks if version is 2.0.0 range and then replaces it with 2.0.0
    // for compatibility
    static const std::regex versionRegex(
        R"(\"_version\"\s*:\s*\"(2\.\d\.\d)\")",
        std::regex_constants::ECMAScript | std::regex_constants::optimize);

    std::smatch matches;
    std::string cppStr(stringData);

    std::string sub(cppStr.substr(0, 100));

    DEBUG("First 100 chars from json: {}", sub);

    if (std::regex_search(cppStr, matches, versionRegex)) {
        // Does not match supported version
        if (matches.size() >= 1) {

            // match group is index 1 because we're matching for (2.x.x)
            auto badVersion = matches[1].str();
            DEBUG("Performing fixup for version {}", badVersion);

            // mutates the string does not copy
            cppStr.replace(matches[1].first, matches[1].second, "2.0.0");

            original = StandardLevelInfoSaveData_DeserializeFromJSONString(
                StringW(cppStr));
        }
    }
    // }

    if (!original || !original.ptr()) {
        DEBUG("No fixup performed for map version");
        original = StandardLevelInfoSaveData_DeserializeFromJSONString(stringData);
    }

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

        DEBUG("Handling set {}", i);
        auto originalBeatmapSet = original->difficultyBeatmapSets[i];
        auto customBeatmaps = ArrayW<GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmap *>(originalBeatmapSet->difficultyBeatmaps.size());

        auto const& difficultyBeatmaps = beatmapSetJson.FindMember(u"_difficultyBeatmaps")->value;

        for (rapidjson::SizeType j = 0; j < originalBeatmapSet->difficultyBeatmaps.size(); j++) {
            DEBUG("Handling map {}", j);

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

        customBeatmapSets[i] = GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmapSet::New_ctor(
            originalBeatmapSet->beatmapCharacteristicName,
            customBeatmaps
        );
    }
    DEBUG("{}", customSaveData->ToString());
    return customSaveData;
}

MAKE_AUTO_HOOK_ORIG_MATCH(SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels, &GlobalNamespace::SinglePlayerLevelSelectionFlowCoordinator::get_enableCustomLevels, bool, GlobalNamespace::SinglePlayerLevelSelectionFlowCoordinator* self) {
    DEBUG("SinglePlayerLevelSelectionFlowCoordinator_get_enableCustomLevels override returning true");
    return true;
}
