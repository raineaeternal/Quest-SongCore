#pragma once

#include "./_config.h"
#include "Capabilities.hpp"
#include <stdexcept>

namespace SongCore {
    /// @brief Registers a capability to let songcore know what the game is capable of
    /// @throws std::runtime_error if SongCore::Capabilities::get_instance() returns nullptr
    /// @param capability The capability to register
    inline void RegisterCapability(std::string_view capability) {
        auto instance = SongCore::Capabilities::get_instance();
        if (!instance) [[unlikely]] throw std::runtime_error("SongCore::Capabilities static instance was null in SongCore::RegisterCapability!");
        instance->RegisterCapability(capability);
    }

    /// @brief Unregisters a capability to let songcore know what the game is capable of
    /// @throws std::runtime_error if SongCore::Capabilities::get_instance() returns nullptr
    /// @param capability The capability to register
    inline void UnregisterCapability(std::string_view capability) {
        auto instance = SongCore::Capabilities::get_instance();
        if (!instance) [[unlikely]] throw std::runtime_error("SongCore::Capabilities static instance was null in SongCore::UnregisterCapability!");
        instance->UnregisterCapability(capability);
    }

    /// @brief checks whether a capability is registered
    /// @param capability The checked capability
    /// @throws std::runtime_error if SongCore::Capabilities::get_instance() returns nullptr
    /// @return true for registered, false for not
    inline bool IsCapabilityRegistered(std::string_view capability) {
        auto instance = SongCore::Capabilities::get_instance();
        if (!instance) [[unlikely]] throw std::runtime_error("SongCore::Capabilities static instance was null in SongCore::IsCapabilityRegistered!");
        return instance->IsCapabilityRegistered(std::move(capability));
    }

    /// @brief provides access to the registered capabilities without allowing edits
    /// @throws std::runtime_error if SongCore::Capabilities::get_instance() returns nullptr
    inline std::span<const std::string> GetRegisteredCapabilities() {
        auto instance = SongCore::Capabilities::get_instance();
        if (!instance) [[unlikely]] throw std::runtime_error("SongCore::Capabilities static instance was null in SongCore::GetRegisteredCapabilities!");
        return instance->GetRegisteredCapabilities();
    }

    /// @brief provides access to an event that gets invoked when the capabilities are updated. not guaranteed to run on main thread! cleared on soft restart. Invoked after the particular capability is added to the list.
    /// @throws std::runtime_error if SongCore::Capabilities::get_instance() returns nullptr
    inline UnorderedEventCallback<std::string_view, Capabilities::CapabilityEventKind>& GetCapabilitiesUpdatedEvent() {
        auto instance = SongCore::Capabilities::get_instance();
        if (!instance) [[unlikely]] throw std::runtime_error("SongCore::Capabilities static instance was null in SongCore::GetCapabilitiesUpdatedEvent!");
        return instance->GetCapabilitiesUpdatedEvent();
    }

}
