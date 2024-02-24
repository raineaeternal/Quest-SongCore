#include "SongCore.hpp"
#include "logging.hpp"

namespace SongCore::API {
    namespace Capabilities {
        static UnorderedEventCallback<std::string_view, Capabilities::CapabilityEventKind> _capabilitiesUpdated;
        static std::vector<std::string> _registeredCapabilities;

        void RegisterCapability(std::string_view capability) {
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

        void UnregisterCapability(std::string_view capability) {
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

        bool IsCapabilityRegistered(std::string_view capability) {
            auto itr = std::find_if(
                _registeredCapabilities.begin(),
                _registeredCapabilities.end(),
                [&capability](auto v){ return v == capability; }
            );

            // if itr != end, that means it was found in the vector
            return itr != _registeredCapabilities.end();
        }

        std::span<const std::string> GetRegisteredCapabilities() {
            return _registeredCapabilities;
        }

        UnorderedEventCallback<std::string_view, Capabilities::CapabilityEventKind>& GetCapabilitiesUpdatedEvent() {
            return _capabilitiesUpdated;
        }
    }
}
