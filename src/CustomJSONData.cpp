#include "CustomJSONData.hpp"
#include "paper/shared/utfcpp/source/utf8.h"
#include <cctype>
#include <string>

using namespace GlobalNamespace;

DEFINE_TYPE(SongCore::CustomJSONData, CustomLevelInfoSaveData);
DEFINE_TYPE(SongCore::CustomJSONData, CustomDifficultyBeatmap);

namespace SongCore::CustomJSONData {

	void CustomLevelInfoSaveData::ctor(
		StringW songName,
		StringW songSubName,
		StringW songAuthorName,
		StringW levelAuthorName,
		float beatsPerMinute,
		float songTimeOffset,
		float shuffle,
		float shufflePeriod,
		float previewStartTime,
		float previewDuration,
		StringW songFilename,
		StringW coverImageFilename,
		StringW environmentName,
		StringW allDirectionsEnvironmentName,
		ArrayW<::StringW> environmentNames,
		ArrayW<GlobalNamespace::BeatmapLevelColorSchemeSaveData*> colorSchemes,
		ArrayW<GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmapSet*> difficultyBeatmapSets
	) {
		INVOKE_CTOR();

		_ctor(
			songName,
			songSubName,
			songAuthorName,
			levelAuthorName,
			beatsPerMinute,
			songTimeOffset,
			shuffle,
			shufflePeriod,
			previewStartTime,
			previewDuration,
			songFilename,
			coverImageFilename,
			environmentName,
			allDirectionsEnvironmentName,
			environmentNames,
			colorSchemes,
			difficultyBeatmapSets
		);
	}

	std::optional<std::reference_wrapper<const CustomLevelInfoSaveData::BasicCustomLevelDetails>> CustomLevelInfoSaveData::TryGetBasicLevelDetails() {
		ParseLevelDetails();
		return _cachedLevelDetails;
	}

	bool CustomLevelInfoSaveData::TryGetBasicLevelDetails(BasicCustomLevelDetails& outDetails) {
		if (ParseLevelDetails()) {
			outDetails = _cachedLevelDetails.value();
			return true;
		}
		return false;
	}

	std::optional<std::reference_wrapper<CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetailsSet const>> CustomLevelInfoSaveData::TryGetCharacteristic(std::string const& characteristic) {
		if (ParseLevelDetails()) {
			return _cachedLevelDetails->TryGetCharacteristic(characteristic);
		}
		return std::nullopt;
	}

	bool CustomLevelInfoSaveData::TryGetCharacteristic(std::string const& characteristic, CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetailsSet& outSet) {
		if (ParseLevelDetails()) {
			return _cachedLevelDetails->TryGetCharacteristic(characteristic, outSet);
		}
		return false;
	}

	std::optional<std::reference_wrapper<CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetails const>> CustomLevelInfoSaveData::TryGetCharacteristicAndDifficulty(std::string const& characteristic, GlobalNamespace::BeatmapDifficulty difficulty) {
		if (ParseLevelDetails()) {
			return _cachedLevelDetails->TryGetCharacteristicAndDifficulty(characteristic, difficulty);
		}
		return std::nullopt;
	}

	bool CustomLevelInfoSaveData::TryGetCharacteristicAndDifficulty(std::string const& characteristic, GlobalNamespace::BeatmapDifficulty difficulty, CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetails& outDetails) {
		if (ParseLevelDetails()) {
			return _cachedLevelDetails->TryGetCharacteristicAndDifficulty(characteristic, difficulty, outDetails);
		}
		return false;
	}

	bool CustomLevelInfoSaveData::ParseLevelDetails() {
		if (_cachedLevelDetails.has_value()) return true;
		BasicCustomLevelDetails levelDetails;

		if (!levelDetails.Deserialize(doc->GetObject())) return false;

		_cachedLevelDetails = std::move(levelDetails);
		return true;
	}

	std::optional<std::reference_wrapper<CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetailsSet const>> CustomLevelInfoSaveData::BasicCustomLevelDetails::TryGetCharacteristic(std::string const& characteristic) const {
		auto charItr = characteristicNameToBeatmapDetailsSet.find(characteristic);
		if (charItr != characteristicNameToBeatmapDetailsSet.end()) return charItr->second;
		return std::nullopt;
	}

	bool CustomLevelInfoSaveData::BasicCustomLevelDetails::TryGetCharacteristic(std::string const& characteristic, CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetailsSet& outDetailsSet) const {
		auto charItr = characteristicNameToBeatmapDetailsSet.find(characteristic);
		if (charItr != characteristicNameToBeatmapDetailsSet.end()) {
			outDetailsSet = charItr->second;
			return true;
		}
		return false;
	}

	std::optional<std::reference_wrapper<CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetails const>> CustomLevelInfoSaveData::BasicCustomLevelDetails::TryGetCharacteristicAndDifficulty(std::string const& characteristic, GlobalNamespace::BeatmapDifficulty difficulty) const {
		auto charItr = characteristicNameToBeatmapDetailsSet.find(characteristic);
		if (charItr != characteristicNameToBeatmapDetailsSet.end()) return charItr->second.TryGetDifficulty(difficulty);
		return std::nullopt;
	}

	bool CustomLevelInfoSaveData::BasicCustomLevelDetails::TryGetCharacteristicAndDifficulty(std::string const& characteristic, GlobalNamespace::BeatmapDifficulty difficulty, BasicCustomDifficultyBeatmapDetails& outDetails) const {
		auto charItr = characteristicNameToBeatmapDetailsSet.find(characteristic);
		if (charItr != characteristicNameToBeatmapDetailsSet.end()) return charItr->second.TryGetDifficulty(difficulty, outDetails);
		return false;
	}

	static void ParseContributorArrayInto(ValueUTF16 const& customData, std::vector<CustomLevelInfoSaveData::BasicCustomLevelDetails::Contributor>& out) {
		auto arrayItr = customData.FindMember(u"_contributors");
		if (arrayItr == customData.MemberEnd() || !arrayItr->value.IsArray()) return;
		for (auto& value : arrayItr->value.GetArray()) {
			out.emplace_back().Deserialize(value);
		}
	}

	// this is all the characteristics -> diff sets
	bool CustomLevelInfoSaveData::BasicCustomLevelDetails::Deserialize(ValueUTF16 const& value) {
		auto memberEnd = value.MemberEnd();
		bool foundEverything = true;
		auto difficultyBeatmapSetsItr = value.FindMember(u"_difficultyBeatmapSets");
		if (difficultyBeatmapSetsItr != memberEnd && difficultyBeatmapSetsItr->value.IsArray()) {
			// check each set
			for (auto& set : difficultyBeatmapSetsItr->value.GetArray()) {
				auto characteristicName = utf8::utf16to8(set[u"_beatmapCharacteristicName"].Get<std::u16string>());
				auto& diffSet = characteristicNameToBeatmapDetailsSet[characteristicName];
				diffSet.characteristicName = characteristicName;
				diffSet.Deserialize(set);
			}
		} else {
			foundEverything = false;
		}

		auto customDataItr = value.FindMember(u"_customData");
		if (customDataItr != memberEnd && customDataItr->value.IsObject()) {
			ParseContributorArrayInto(customDataItr->value, contributors);
		}

		return foundEverything;
	}

	bool CustomLevelInfoSaveData::BasicCustomLevelDetails::Contributor::Deserialize(ValueUTF16 const& value) {
		auto memberEnd = value.MemberEnd();
		auto nameItr = value.FindMember(u"_name");
		if (nameItr != memberEnd && nameItr->value.IsString()) name = utf8::utf16to8(nameItr->value.Get<std::u16string>());
		auto roleItr = value.FindMember(u"_role");
		if (roleItr != memberEnd && roleItr->value.IsString()) role = utf8::utf16to8(roleItr->value.Get<std::u16string>());
		auto iconPathItr = value.FindMember(u"_iconPath");
		if (iconPathItr != memberEnd && iconPathItr->value.IsString()) iconPath = utf8::utf16to8(iconPathItr->value.Get<std::u16string>());

		return true;
	}

	std::optional<std::reference_wrapper<CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetails const>> CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetailsSet::TryGetDifficulty(GlobalNamespace::BeatmapDifficulty difficulty) const {
		auto diffItr = difficultyToDifficultyBeatmapDetails.find(difficulty);
		if (diffItr != difficultyToDifficultyBeatmapDetails.end()) return diffItr->second;
		return std::nullopt;
	}

	bool CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetailsSet::TryGetDifficulty(GlobalNamespace::BeatmapDifficulty difficulty, CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetails& outDetails) const {
		auto diffItr = difficultyToDifficultyBeatmapDetails.find(difficulty);
		if (diffItr != difficultyToDifficultyBeatmapDetails.end()) {
			outDetails = diffItr->second;
			return true;
		}
		return false;
	}

	GlobalNamespace::BeatmapDifficulty ParseDiff(std::string_view diffName) {
		static std::pair<std::string, GlobalNamespace::BeatmapDifficulty> diffNameToDiffMap[6] {
			{ "Easy", GlobalNamespace::BeatmapDifficulty::Easy },
			{ "Normal", GlobalNamespace::BeatmapDifficulty::Normal },
			{ "Hard", GlobalNamespace::BeatmapDifficulty::Hard },
			{ "Expert", GlobalNamespace::BeatmapDifficulty::Expert },
			{ "ExpertPlus", GlobalNamespace::BeatmapDifficulty::ExpertPlus },
		};

		for (auto& [name, diff] : diffNameToDiffMap) {
			if (name == diffName) {
				return diff;
			}
		}

		return GlobalNamespace::BeatmapDifficulty::Easy;
	}

	// this is the diff set -> difficulties
	bool CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetailsSet::Deserialize(ValueUTF16 const& value) {
		auto characteristicLabelItr = value.FindMember(u"_characteristicLabel");
		if (characteristicLabelItr != value.MemberEnd()) characteristicLabel = utf8::utf16to8(characteristicLabelItr->value.Get<std::u16string>());

		auto characteristicIconImageFileNameItr = value.FindMember(u"_characteristicIconImageFileName");
		if (characteristicIconImageFileNameItr != value.MemberEnd()) characteristicIconImageFileName = utf8::utf16to8(characteristicIconImageFileNameItr->value.Get<std::u16string>());

		auto difficultyBeatmapsItr = value.FindMember(u"_difficultyBeatmaps");
		if (difficultyBeatmapsItr == value.MemberEnd() || !difficultyBeatmapsItr->value.IsArray()) return false;

		// check each beatmap
		for (auto& beatmap : difficultyBeatmapsItr->value.GetArray()) {
			auto diffName = utf8::utf16to8(beatmap[u"_difficulty"].Get<std::u16string>());
			GlobalNamespace::BeatmapDifficulty diff = ParseDiff(diffName);

			auto& diffData = difficultyToDifficultyBeatmapDetails[diff];
			diffData.characteristicName = characteristicName;
			diffData.difficulty = diff;
			diffData.Deserialize(beatmap);
		}

		return false;
	}

	static void ParseSimpleStringArrayInto(ValueUTF16 const& customData, std::u16string_view valueName, std::vector<std::string>& out) {
		auto arrayItr = customData.FindMember(valueName.data());
		if (arrayItr == customData.MemberEnd() || !arrayItr->value.IsArray()) return;
		for (auto& value : arrayItr->value.GetArray()) {
			out.emplace_back(utf8::utf16to8(value.Get<std::u16string>()));
		}
	}

	// this is each difficulty individually
	bool CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetails::Deserialize(ValueUTF16 const& value) {
		auto customDataItr = value.FindMember(u"_customData");
		if (customDataItr == value.MemberEnd()) return false;
		auto& customData = customDataItr->value;
		auto memberEnd = customData.MemberEnd();

		auto environmentTypeItr = customData.FindMember(u"_environmentType");
		if (environmentTypeItr != memberEnd) environmentType = utf8::utf16to8(environmentTypeItr->value.Get<std::u16string>());

		auto showRotationNoteSpawnLinesItr = customData.FindMember(u"_showRotationNoteSpawnLines");
		if (showRotationNoteSpawnLinesItr != memberEnd) showRotationNoteSpawnLines = showRotationNoteSpawnLinesItr->value.GetBool();

		auto oneSaberItr = customData.FindMember(u"_oneSaber");
		if (oneSaberItr != memberEnd) oneSaber = oneSaberItr->value.GetBool();

		ParseSimpleStringArrayInto(customData, u"_requirements", requirements);
		ParseSimpleStringArrayInto(customData, u"_suggestions", suggestions);
		ParseSimpleStringArrayInto(customData, u"_warnings", warnings);
		ParseSimpleStringArrayInto(customData, u"_information", information);

		CustomColors customColors;
		// if any custom colors are deserialized, set the value
		if (customColors.Deserialize(customData)) this->customColors = std::move(customColors);

		return true;
	}

	static UnityEngine::Color DeserializeColor(ValueUTF16 const& value) {
		auto memberEnd = value.MemberEnd();
		float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
		auto rItr = value.FindMember(u"r");
		if (rItr != memberEnd && rItr->value.IsNumber()) r = rItr->value.GetFloat();
		auto gItr = value.FindMember(u"g");
		if (gItr != memberEnd && gItr->value.IsNumber()) g = gItr->value.GetFloat();
		auto bItr = value.FindMember(u"b");
		if (bItr != memberEnd && bItr->value.IsNumber()) b = bItr->value.GetFloat();
		auto aItr = value.FindMember(u"a");
		if (aItr != memberEnd && aItr->value.IsNumber()) a = aItr->value.GetFloat();

		return {r, g, b, a};
	}

	#define DESERIALIZE_OPT_COLOR(color_) do {							\
		auto itr = value.FindMember(u"_" u## #color_);                  \
		if (itr != memberEnd && itr->value.IsObject()) {                \
			color_ = DeserializeColor(itr->value);                      \
			foundAnything = true;                                       \
		}                                                               \
	} while (0)

    bool CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetails::CustomColors::Deserialize(ValueUTF16 const& value) {
		auto memberEnd = value.MemberEnd();
		bool foundAnything = false;
		DESERIALIZE_OPT_COLOR(colorLeft);
		DESERIALIZE_OPT_COLOR(colorRight);
		DESERIALIZE_OPT_COLOR(envColorRight);
		DESERIALIZE_OPT_COLOR(envColorLeft);
		DESERIALIZE_OPT_COLOR(envColorWhite);
		DESERIALIZE_OPT_COLOR(envColorLeftBoost);
		DESERIALIZE_OPT_COLOR(envColorRightBoost);
		DESERIALIZE_OPT_COLOR(envColorWhiteBoost);
		DESERIALIZE_OPT_COLOR(obstacleColor);

		return foundAnything;
	}

void CustomDifficultyBeatmap::ctor(
	StringW difficultyName,
	int difficultyRank,
	StringW beatmapFilename,
	float noteJumpMovementSpeed,
	float noteJumpStartBeatOffset,
	int beatmapColorSchemeIdx,
	int environmentNameIdx
) {
	INVOKE_CTOR();

	_ctor(
		difficultyName,
		difficultyRank,
		beatmapFilename,
		noteJumpMovementSpeed,
		noteJumpStartBeatOffset,
		beatmapColorSchemeIdx,
		environmentNameIdx
	);
}
}
