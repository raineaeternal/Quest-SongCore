#include "Capabilities.hpp"
#include "logging.hpp"

DEFINE_TYPE(SongCore, Capabilities);

namespace SongCore {
    Capabilities* Capabilities::_instance = nullptr;
    std::vector<std::string> Capabilities::_registeredCapabilities = {};

    void Capabilities::Initialize() {
        _instance = this;
    }

    void Capabilities::Dispose() {
        _instance = nullptr;
    }

    void Capabilities::RegisterCapability(std::string_view capability) {
        auto itr = std::find_if(
            _registeredCapabilities.begin(),
            _registeredCapabilities.end(),
            [&capability](auto v){ return v == capability; }
        );

        if (itr == _registeredCapabilities.end()) {
            _registeredCapabilities.emplace_back(capability);
            _capabilitiesUpdated.invoke(capability, CapabilityEventKind::Registered);
        } else {
            WARNING("Capability '{}' was registered more than once! not registering again", capability);
        }
    }

    void Capabilities::UnregisterCapability(std::string_view capability) {
        auto itr = std::find_if(
            _registeredCapabilities.begin(),
            _registeredCapabilities.end(),
            [&capability](auto v){ return v == capability; }
        );

        if (itr != _registeredCapabilities.end()) {
            _registeredCapabilities.erase(itr);
            _capabilitiesUpdated.invoke(capability, CapabilityEventKind::Unregistered);
        } else {
            WARNING("Capability '{}' was unregistered more than once! not unregistering again", capability);
        }
    }

    bool Capabilities::IsCapabilityRegistered(std::string_view capability) const {
        auto itr = std::find_if(
            _registeredCapabilities.begin(),
            _registeredCapabilities.end(),
            [&capability](auto v){ return v == capability; }
        );

        // if itr != end, that means it was found in the vector
        return itr != _registeredCapabilities.end();
    }

    std::span<const std::string> Capabilities::GetRegisteredCapabilities() const {
        return _registeredCapabilities;
    }

    UnorderedEventCallback<std::string_view, Capabilities::CapabilityEventKind>& Capabilities::GetCapabilitiesUpdatedEvent() {
        return _capabilitiesUpdated;
    }
}
