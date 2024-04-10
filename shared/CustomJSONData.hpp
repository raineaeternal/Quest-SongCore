#pragma once

#include <functional>
#include <memory>
#include <optional>

#include "custom-types/shared/macros.hpp"
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/document.h"

#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "BeatmapLevelSaveDataVersion4/BeatmapLevelSaveData.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "UnityEngine/Color.hpp"

namespace SongCore::CustomJSONData {
using ValueUTF16 = rapidjson::GenericValue<rapidjson::UTF16<char16_t>>;
using DocumentUTF16 = rapidjson::GenericDocument<rapidjson::UTF16<char16_t>>;
}

DECLARE_CLASS_CODEGEN(SongCore::CustomJSONData, CustomLevelInfoSaveData, GlobalNamespace::StandardLevelInfoSaveData,

    DECLARE_CTOR(ctor,
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
    );
    DECLARE_SIMPLE_DTOR();

public:
    std::shared_ptr<DocumentUTF16> doc;
    std::optional<std::reference_wrapper<const ValueUTF16>> customData;

	/// @brief struct providing basic information about a difficulty beatmap (characteristic + difficulty)
	struct BasicCustomDifficultyBeatmapDetails {
		struct CustomColors {
			std::optional<UnityEngine::Color> colorLeft;
			std::optional<UnityEngine::Color> colorRight;
			std::optional<UnityEngine::Color> envColorRight;
			std::optional<UnityEngine::Color> envColorLeft;
			std::optional<UnityEngine::Color> envColorWhite;
			std::optional<UnityEngine::Color> envColorLeftBoost;
			std::optional<UnityEngine::Color> envColorRightBoost;
			std::optional<UnityEngine::Color> envColorWhiteBoost;
			std::optional<UnityEngine::Color> obstacleColor;

			/// @brief deserializer method
			/// @return since everything is completely optional, returns true if anything was found, false if nothing was found
			bool Deserialize(ValueUTF16 const& value);
		};

		/// @brief characteristic name as parsed from info.dat
		std::string characteristicName;
		/// @brief difficulty enum as parsed from info.dat
		GlobalNamespace::BeatmapDifficulty difficulty;

		/// @brief requirements to play this beatmap
		std::vector<std::string> requirements;
		/// @brief suggestions for this beatmap
		std::vector<std::string> suggestions;
		/// @brief warnings for this beatmap
		std::vector<std::string> warnings;
		/// @brief information for this beatmap
		std::vector<std::string> information;

		/// @brief if set, the custom diff name
		std::optional<std::string> customDiffName;
		/// @brief if set, the custom diff hover text
		std::optional<std::string> customDiffLabel;
		/// @brief if set, the environment type
		std::optional<std::string> environmentType;
		/// @brief if set, whether this map overrides saber count for characteristic
		std::optional<bool> oneSaber;
		/// @brief if set, the custom colors for this map
		std::optional<CustomColors> customColors;
		/// @brief whether to show rotation spawn lines
		std::optional<bool> showRotationNoteSpawnLines;

		/// @brief deserializer method
		/// @return whether deserialization was succesful
		bool Deserialize(ValueUTF16 const& value);
	};

	/// @brief struct providing basic information about a difficulty beatmap set (characteristic)
	struct BasicCustomDifficultyBeatmapDetailsSet {
		/// @brief map of GlobalNamespace::BeatmapDifficulty to BasicCustomDifficultyBeatmapDetails
		std::unordered_map<GlobalNamespace::BeatmapDifficulty::__BeatmapDifficulty_Unwrapped, BasicCustomDifficultyBeatmapDetails> difficultyToDifficultyBeatmapDetails;
		/// @brief characteristic name as parsed from info.dat
		std::string characteristicName;
		/// @brief optional custom label (hover text)
		std::optional<std::string> characteristicLabel;
		/// @brief optional custom icon filename (combine with customLevelPath)
		std::optional<std::string> characteristicIconImageFileName;

		/// @brief tries to get the difficulty details for a characteristic
		/// @param characteristic characteristic name
		/// @param difficulty the difficulty to get the details for
		/// @return optional details, nullopt if either characteristic or difficulty not found
		[[nodiscard]] std::optional<std::reference_wrapper<BasicCustomDifficultyBeatmapDetails const>> TryGetDifficulty(GlobalNamespace::BeatmapDifficulty difficulty) const;

		/// @brief tries to get the difficulty details for a characteristic
		/// @param difficulty the difficulty to get the details for
		/// @param outDetails reference to your output variable. will copy construct
		/// @return true if found, false if not
		[[nodiscard]] bool TryGetDifficulty(GlobalNamespace::BeatmapDifficulty difficulty, BasicCustomDifficultyBeatmapDetails& outDetails) const;

		/// @brief deserializer method
		/// @return whether deserialization was succesful
		bool Deserialize(ValueUTF16 const& value);
	};

	/// @brief struct providing basic information about a beatmap
	struct BasicCustomLevelDetails {
		/// @brief struct describing a contributor entry
		struct Contributor {
			/// @brief name of this contributor
			std::string name;
			/// @brief what did they do?
			std::string role;
			/// @brief path to the icon to display for this contributor
			std::filesystem::path iconPath;

			/// @brief deserializer method
			/// @return whether deserialization was succesful
			bool Deserialize(ValueUTF16 const& value);
		};

		std::unordered_map<std::string, BasicCustomDifficultyBeatmapDetailsSet> characteristicNameToBeatmapDetailsSet;

		/// @brief contributors to this level
		std::vector<Contributor> contributors;

		/// @brief tries to get the characteristic details
		/// @param characteristic characteristic name
		/// @return optional details set, nullopt if not found
		[[nodiscard]] std::optional<std::reference_wrapper<BasicCustomDifficultyBeatmapDetailsSet const>> TryGetCharacteristic(std::string const& characteristic) const;

		/// @brief tries to get the characteristic details
		/// @param characteristic characteristic name
		/// @param outSet reference to destination details set. will copy construct
		/// @return true if found, false if not
		[[nodiscard]] bool TryGetCharacteristic(std::string const& characteristic, BasicCustomDifficultyBeatmapDetailsSet& outSet) const;

		/// @brief tries to get the difficulty details for a characteristic
		/// @param characteristic characteristic name
		/// @param difficulty the difficulty to get the details for
		/// @return optional details, nullopt if either characteristic or difficulty not found
		[[nodiscard]] std::optional<std::reference_wrapper<BasicCustomDifficultyBeatmapDetails const>> TryGetCharacteristicAndDifficulty(std::string const& characteristic, GlobalNamespace::BeatmapDifficulty difficulty) const;

		/// @brief tries to get the difficulty details for a characteristic
		/// @param characteristic characteristic name
		/// @param difficulty the difficulty to get the details for
		/// @param outDetails reference to your output variable. will copy construct
		/// @return true if found, false if not
		[[nodiscard]] bool TryGetCharacteristicAndDifficulty(std::string const& characteristic, GlobalNamespace::BeatmapDifficulty difficulty, BasicCustomDifficultyBeatmapDetails& outDetails) const;

		/// @brief deserializer method
		/// @return whether deserialization was succesful
		bool Deserialize(ValueUTF16 const& value);
	};

	/// @brief tries to get the basic level details for this savedata
	/// @return optional reference to the parsed data, or nullopt if parse was unsuccesful
	std::optional<std::reference_wrapper<BasicCustomLevelDetails const>> TryGetBasicLevelDetails();

	/// @brief tries to get the basic level details for this savedata
	/// @param outDetails reference to your output variable. will copy construct
	/// @return true if available, false if not
	bool TryGetBasicLevelDetails(BasicCustomLevelDetails& outDetails);

	/// @brief tries to get the characteristic details
	/// @param characteristic characteristic name
	/// @return optional details set, nullopt if not found
	[[nodiscard]] std::optional<std::reference_wrapper<BasicCustomDifficultyBeatmapDetailsSet const>> TryGetCharacteristic(std::string const& characteristic);

	/// @brief tries to get the characteristic details
	/// @param characteristic characteristic name
	/// @param outSet reference to destination details set. will copy construct
	/// @return true if found, false if not
	[[nodiscard]] bool TryGetCharacteristic(std::string const& characteristic, BasicCustomDifficultyBeatmapDetailsSet& outSet);
	/// @brief tries to get the difficulty details for a characteristic
	/// @param characteristic characteristic name
	/// @param difficulty the difficulty to get the details for
	/// @return optional details, nullopt if either characteristic or difficulty not found
	[[nodiscard]] std::optional<std::reference_wrapper<BasicCustomDifficultyBeatmapDetails const>> TryGetCharacteristicAndDifficulty(std::string const& characteristic, GlobalNamespace::BeatmapDifficulty difficulty);

	/// @brief tries to get the difficulty details for a characteristic
	/// @param characteristic characteristic name
	/// @param difficulty the difficulty to get the details for
	/// @param outDetails reference to your output variable. will copy construct
	/// @return true if found, false if not
	[[nodiscard]] bool TryGetCharacteristicAndDifficulty(std::string const& characteristic, GlobalNamespace::BeatmapDifficulty difficulty, BasicCustomDifficultyBeatmapDetails& outDetails);

private:
	bool ParseLevelDetails();
	std::optional<BasicCustomLevelDetails> _cachedLevelDetails;
)

DECLARE_CLASS_CODEGEN(SongCore::CustomJSONData, CustomDifficultyBeatmapSet, GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmapSet,

	DECLARE_CTOR(ctor,
		StringW beatmapCharacteristicName,
		ArrayW<GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmap*> difficultyBeatmaps
	);
public:
	std::optional<std::reference_wrapper<const ValueUTF16>> customData;
)

DECLARE_CLASS_CODEGEN(SongCore::CustomJSONData, CustomDifficultyBeatmap, GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmap,

	DECLARE_CTOR(ctor,
		StringW difficultyName,
		int difficultyRank,
		StringW beatmapFilename,
		float noteJumpMovementSpeed,
		float noteJumpStartBeatOffset,
		int beatmapColorSchemeIdx,
		int environmentNameIdx
	);

public:
	std::optional<std::reference_wrapper<const ValueUTF16>> customData;
)

DECLARE_CLASS_CODEGEN(SongCore::CustomJSONData, CustomBeatmapLevelSaveData, BeatmapLevelSaveDataVersion4::BeatmapLevelSaveData,

)
