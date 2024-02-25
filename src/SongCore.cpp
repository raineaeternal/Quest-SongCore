#include "SongCore.hpp"
#include "logging.hpp"

#include "UnityEngine/HideFlags.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Texture.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/TextureWrapMode.hpp"

static inline UnityEngine::HideFlags operator |(UnityEngine::HideFlags a, UnityEngine::HideFlags b) {
    return UnityEngine::HideFlags(a.value__ | b.value__);
}

namespace SongCore::API {
    namespace Capabilities {
        static UnorderedEventCallback<std::string_view, Capabilities::CapabilityEventKind> _capabilitiesUpdated;
        std::mutex _registeredCapabilitiesMutex;
        static std::vector<std::string> _registeredCapabilities;

        void RegisterCapability(std::string_view capability) {
            std::lock_guard<std::mutex> lock(_registeredCapabilitiesMutex);

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
            std::lock_guard<std::mutex> lock(_registeredCapabilitiesMutex);

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
            std::lock_guard<std::mutex> lock(_registeredCapabilitiesMutex);

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

    namespace Characteristics {
        static SafePtr<System::Collections::Generic::List_1<GlobalNamespace::BeatmapCharacteristicSO*>> _registeredCharacteristics;
        static UnorderedEventCallback<GlobalNamespace::BeatmapCharacteristicSO*, Characteristics::CharacteristicEventKind> _characteristicsUpdatedEvent;

        ListW<GlobalNamespace::BeatmapCharacteristicSO*> get_registeredCharacteristics() {
            if (_registeredCharacteristics) return _registeredCharacteristics.ptr();
            _registeredCharacteristics = System::Collections::Generic::List_1<GlobalNamespace::BeatmapCharacteristicSO*>::New_ctor();
            return _registeredCharacteristics.ptr();
        }

        void RegisterCustomCharacteristic(GlobalNamespace::BeatmapCharacteristicSO* characteristic) {
            characteristic->hideFlags = characteristic->hideFlags | UnityEngine::HideFlags::DontUnloadUnusedAsset;

            if (GetCharacteristicBySerializedName(static_cast<std::string>(characteristic->serializedName))) {
                get_registeredCharacteristics()->Add(characteristic);
                _characteristicsUpdatedEvent.invoke(characteristic, CharacteristicEventKind::Registered);
            } else {
                WARNING("Characteristic '{}' was registered more than once! not registering again", characteristic->serializedName);
            }
        }

        void UnregisterCustomCharacteristic(GlobalNamespace::BeatmapCharacteristicSO* characteristic) {
            auto idx = get_registeredCharacteristics()->IndexOf(characteristic);
            if (idx >= 0) {
                get_registeredCharacteristics()->RemoveAt(idx);
                _characteristicsUpdatedEvent.invoke(characteristic, CharacteristicEventKind::Unregistered);
            } else {
                WARNING("Characteristic '{}' was unregistered more than once! not unregistering again", characteristic->serializedName);
            }
        }

        std::span<GlobalNamespace::BeatmapCharacteristicSO*> GetRegisteredCharacteristics() {
            return get_registeredCharacteristics().ref_to();
        }

        GlobalNamespace::BeatmapCharacteristicSO* GetCharacteristicBySerializedName(std::string_view serializedName) {
            auto characteristic = get_registeredCharacteristics().find([&serializedName](auto x) { return x->serializedName == serializedName; });
            // value_or(nullptr) doesn't work because it's a reference wrapper in an optional
            return characteristic.has_value() ? characteristic.value() : nullptr;
        }

        UnorderedEventCallback<GlobalNamespace::BeatmapCharacteristicSO*, Characteristics::CharacteristicEventKind>& GetCharacteristicsUpdatedEvent() {
            return _characteristicsUpdatedEvent;
        }

        GlobalNamespace::BeatmapCharacteristicSO* CreateCharacteristic(UnityEngine::Sprite* icon, StringW characteristicName, StringW hintText, StringW serializedName, StringW compoundIdPartName, bool requires360Movement, bool containsRotationEvents, int sortingOrder) {
            icon->texture->wrapMode = UnityEngine::TextureWrapMode::Clamp;

            auto characteristic = UnityEngine::ScriptableObject::CreateInstance<GlobalNamespace::BeatmapCharacteristicSO*>();
            characteristic->hideFlags = characteristic->hideFlags | UnityEngine::HideFlags::DontUnloadUnusedAsset;
            characteristic->_icon = icon;
            characteristic->_descriptionLocalizationKey = hintText;
            characteristic->_serializedName = serializedName;
            characteristic->_characteristicNameLocalizationKey = characteristicName;
            characteristic->_compoundIdPartName = compoundIdPartName;
            characteristic->_requires360Movement = requires360Movement;
            characteristic->_containsRotationEvents = containsRotationEvents;
            characteristic->_sortingOrder = sortingOrder;

            return characteristic;
        }
    }
}
