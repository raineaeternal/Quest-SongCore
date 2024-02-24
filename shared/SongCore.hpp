#pragma once

#include "./_config.h"
#include <stdexcept>
#include "beatsaber-hook/shared/utils/typedefs.h"

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
}
