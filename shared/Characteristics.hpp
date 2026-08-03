#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

#include "custom-types/shared/macros.hpp"
#include "./SongCore.hpp"

#include "GlobalNamespace/MainSystemInit.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollection.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapCharacteristic.hpp"
#include "GlobalNamespace/AppStaticSettingsSO.hpp"
#include "Zenject/IInitializable.hpp"
#include "System/IDisposable.hpp"

/// @brief class that manages the characteristics, including custom ones, and
/// provides access to them for other parts of the mod This is the single source
/// of truth for characteristics, and is the only place that knows about both
/// built-in and custom characteristics. It is a Zenject singleton,
/// and can be accessed through injection or through Characteristics::get_instance().
DECLARE_CLASS_CODEGEN_INTERFACES(SongCore, Characteristics, System::Object, Zenject::IInitializable*, System::IDisposable*) {
        DECLARE_OVERRIDE_METHOD_MATCH(void, Initialize, &Zenject::IInitializable::Initialize);
        DECLARE_OVERRIDE_METHOD_MATCH(void, Dispose, &System::IDisposable::Dispose);

        std::vector<API::Characteristics::CharacteristicInfo> _beatmapCharacteristics;
        std::vector<API::Characteristics::CharacteristicInfo> _disabledBeatmapCharacteristics;
        DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::AppStaticSettingsSO*, _appStaticSettings);

        DECLARE_CTOR(ctor, GlobalNamespace::BeatmapCharacteristicCollection* beatmapCharacteristicCollection, GlobalNamespace::AppStaticSettingsSO* appStaticSettings);
    public:
        /// @brief gets the current instance, for use from places that can't get it through Zenject injection (e.g. hooks). May be null if not yet initialized.
        static Characteristics* get_instance();

        /// @brief Registers a custom characteristic that was already created and registered via SongCore::API::Characteristics::CreateCharacteristic/RegisterCustomCharacteristic, identified by serializedName
        void RegisterCustomCharacteristic(API::Characteristics::CharacteristicInfo characteristic);

        /// @brief Unregisters a custom characteristic, identified by serializedName
        void UnregisterCustomCharacteristic(API::Characteristics::CharacteristicInfo characteristic);

        /// @brief looks up a (possibly custom) characteristic by serialized name
        std::optional<API::Characteristics::CharacteristicInfo> GetCharacteristicBySerializedName(std::string_view serializedName);

        /// @brief looks up info for one of the base game's built-in characteristics
        API::Characteristics::CharacteristicInfo GetCharacteristic(GlobalNamespace::BeatmapCharacteristic characteristic);

        /// @brief provides access to the registered characteristics without allowing edits
        std::vector<API::Characteristics::CharacteristicInfo> GetRegisteredCharacteristics();
        __declspec(property(get=GetRegisteredCharacteristics)) std::vector<API::Characteristics::CharacteristicInfo> RegisteredCharacteristics;

        /// @brief provides access to an event that gets invoked when the custom characteristics are updated. not guaranteed to run on main thread! cleared on soft restart. Invoked after the particular characteristic is added to the list.
        unordered_event_callback<API::Characteristics::CharacteristicInfo, SongCore::API::Characteristics::CharacteristicEventKind>& GetCharacteristicsUpdatedEvent();
        __declspec(property(get=GetCharacteristicsUpdatedEvent)) unordered_event_callback<API::Characteristics::CharacteristicInfo, SongCore::API::Characteristics::CharacteristicEventKind>& CharacteristicsUpdatedEvent;

        /// @brief provides access to enabled characteristics
        std::span<API::Characteristics::CharacteristicInfo> GetEnabledCharacteristics();
        __declspec(property(get=GetEnabledCharacteristics)) std::span<API::Characteristics::CharacteristicInfo> EnabledCharacteristics;

        /// @brief provides access to disabled characteristics (require 360 while the app settings say no 360)
        std::span<API::Characteristics::CharacteristicInfo> GetDisabledCharacteristics();
        __declspec(property(get=GetDisabledCharacteristics)) std::span<API::Characteristics::CharacteristicInfo> DisabledCharacteristics;
    private:
        /// @brief adds the characteristic to the relevant collections
        void AddCharacteristicToCollection(API::Characteristics::CharacteristicInfo characteristic);

        /// @brief removes the characteristic to the relevant collections
        void RemoveCharacteristicFromCollection(API::Characteristics::CharacteristicInfo characteristic);

        /// @brief callback ran when the songcore api is used to update characteristics
        void CharacteristicsUpdated(API::Characteristics::CharacteristicInfo characteristic, SongCore::API::Characteristics::CharacteristicEventKind eventKind);

        unordered_event_callback<API::Characteristics::CharacteristicInfo, SongCore::API::Characteristics::CharacteristicEventKind> _characteristicsUpdated;
        std::mutex _collectionMutex;
};
