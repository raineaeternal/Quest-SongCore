#pragma once

#include "./_config.h"

#include "beatsaber-hook/shared/utils/typedefs.h"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"

#include <span>

namespace SongCore::API {
    namespace Capabilities {
        enum SONGCORE_EXPORT CapabilityEventKind {
            Registered = 0,
            Unregistered = 1,
        };

        /// @brief Registers a capability to let songcore know what the game is capable of
        /// @param capability The capability to register
        SONGCORE_EXPORT void RegisterCapability(std::string_view capability);

        /// @brief Unregisters a capability to let songcore know what the game is capable of
        /// @param capability The capability to register
        SONGCORE_EXPORT void UnregisterCapability(std::string_view capability);

        /// @brief checks whether a capability is registered
        /// @param capability The checked capability
        /// @return true for registered, false for not
        SONGCORE_EXPORT bool IsCapabilityRegistered(std::string_view capability);

        /// @brief provides access to the registered capabilities without allowing edits
        SONGCORE_EXPORT std::span<const std::string> GetRegisteredCapabilities();

        /// @brief provides access to an event that gets invoked when the capabilities are updated. not guaranteed to run on main thread! not cleared on soft restart. Invoked after the particular capability is added to the list.
        SONGCORE_EXPORT UnorderedEventCallback<std::string_view, Capabilities::CapabilityEventKind>& GetCapabilitiesUpdatedEvent();
    }

    namespace Characteristics {
        enum SONGCORE_EXPORT CharacteristicEventKind {
            Registered = 0,
            Unregistered = 1,
        };

        /// @brief method to register a custom characteristic. This has to be ran at late_load at the latest to work correctly. Unregistering is not possible.
        /// @param characteristic the characteristic to register
        SONGCORE_EXPORT void RegisterCustomCharacteristic(GlobalNamespace::BeatmapCharacteristicSO* characteristic);

        /// @brief method to register a custom characteristic. This has to be ran at late_load at the latest to work correctly. Unregistering is not possible.
        /// @param characteristic the characteristic to register
        SONGCORE_EXPORT void UnregisterCustomCharacteristic(GlobalNamespace::BeatmapCharacteristicSO* characteristic);

        /// @brief gets a characteristic by serialized name. Only valid to be called after the first zenject install has happened
        /// @param serializedName the name to look for
        /// @return the found characteristic, or nullptr if not found. not guaranteed to still be a valid unity object!
        SONGCORE_EXPORT GlobalNamespace::BeatmapCharacteristicSO* GetCharacteristicBySerializedName(std::string_view serializedName);

        /// @brief provides access to the registered characteristics without allowing edits
        SONGCORE_EXPORT std::span<GlobalNamespace::BeatmapCharacteristicSO*> GetRegisteredCharacteristics();

        /// @brief provides access to an event that gets invoked when the custom characteristics are updated. not guaranteed to run on main thread! not cleared on soft restart. Invoked after the particular characteristic is added to the list.
        SONGCORE_EXPORT UnorderedEventCallback<GlobalNamespace::BeatmapCharacteristicSO*, Characteristics::CharacteristicEventKind>& GetCharacteristicsUpdatedEvent();
    }
}
