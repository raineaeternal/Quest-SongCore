#include "SongCore.hpp"
#include "SongLoader/RuntimeSongLoader.hpp"
#include "logging.hpp"
#include "config.hpp"

#include "UnityEngine/HideFlags.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Texture.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "UnityEngine/TextureWrapMode.hpp"

#include "GlobalNamespace/BeatmapCharacteristicExtensions.hpp"

#include "beatsaber-hook/shared/safeptr.hpp"

#include <unordered_map>

static inline UnityEngine::HideFlags operator |(UnityEngine::HideFlags a, UnityEngine::HideFlags b) {
    return UnityEngine::HideFlags(a.value__ | b.value__);
}

namespace SongCore::API {
    namespace Capabilities {
        static unordered_event_callback<std::string_view, Capabilities::CapabilityEventKind> _capabilitiesUpdated;
        std::mutex _registeredCapabilitiesMutex;
        static std::vector<std::string> _registeredCapabilities;

        static std::string sanitize(std::string_view capability) {
            std::string sanitized;
            sanitized.reserve(capability.size());
            for (auto c : capability) {
                if (isspace(c)) continue;
                sanitized.push_back(tolower(c));
            }
            return sanitized;
        }

        void RegisterCapability(std::string_view capability) {
            std::lock_guard<std::mutex> lock(_registeredCapabilitiesMutex);

            auto sanitized = sanitize(capability);
            auto itr = std::find(
                _registeredCapabilities.begin(),
                _registeredCapabilities.end(),
                sanitized
            );

            if (itr == _registeredCapabilities.end()) {
                _registeredCapabilities.emplace_back(sanitized);
                _capabilitiesUpdated.invoke(capability, CapabilityEventKind::Registered);
            } else {
                WARNING("Capability '{}' was registered more than once! not registering again", capability);
            }
        }

        void UnregisterCapability(std::string_view capability) {
            std::lock_guard<std::mutex> lock(_registeredCapabilitiesMutex);

            auto sanitized = sanitize(capability);
            auto itr = std::find(
                _registeredCapabilities.begin(),
                _registeredCapabilities.end(),
                sanitized
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

            auto sanitized = sanitize(capability);
            auto itr = std::find(
                _registeredCapabilities.begin(),
                _registeredCapabilities.end(),
                sanitized
            );

            // if itr != end, that means it was found in the vector
            return itr != _registeredCapabilities.end();
        }

        std::span<const std::string> GetRegisteredCapabilities() {
            return _registeredCapabilities;
        }

        unordered_event_callback<std::string_view, Capabilities::CapabilityEventKind>& GetCapabilitiesUpdatedEvent() {
            return _capabilitiesUpdated;
        }
    }

    namespace PlayButton {
        static std::vector<PlayButtonDisablingModInfo> _disablingModInfos;
        static unordered_event_callback<std::span<PlayButtonDisablingModInfo const>> _playButtonDisablingModsChangedEvent;

        void DisablePlayButton(std::string modID, std::string reason) {
            auto itr = std::find_if(_disablingModInfos.begin(), _disablingModInfos.end(), [&modID](auto& x){ return x.modID == modID; });
            if (itr == _disablingModInfos.end()) {
                _disablingModInfos.emplace_back(modID, reason);
                _playButtonDisablingModsChangedEvent.invoke(_disablingModInfos);
            } else {
                WARNING("Mod {} tried disabling the play button twice, which is not supported! current reason: {}, new reason: {}", modID, itr->reason, reason);
            }
        }

        void EnablePlayButton(std::string modID) {
            auto itr = std::find_if(_disablingModInfos.begin(), _disablingModInfos.end(), [&modID](auto& x){ return x.modID == modID; });
            if (itr != _disablingModInfos.end()) {
                _disablingModInfos.erase(itr);
                _playButtonDisablingModsChangedEvent.invoke(_disablingModInfos);
            } else {
                WARNING("Mod {} tried enabling the play button twice, which is not supported!", modID);
            }
        }

        unordered_event_callback<std::span<PlayButtonDisablingModInfo const>>& GetPlayButtonDisablingModsChangedEvent() {
            return _playButtonDisablingModsChangedEvent;
        }

        std::span<PlayButtonDisablingModInfo const> GetPlayButtonDisablingModInfos() {
            return _disablingModInfos;
        }
    }

    // registry for custom characteristics
    namespace Characteristics {
        static std::unordered_map<std::string, CharacteristicInfo> _registeredCharacteristics;
        static unordered_event_callback<CharacteristicInfo, Characteristics::CharacteristicEventKind> _characteristicsUpdatedEvent;

        std::optional<CharacteristicInfo> GetCharacteristicBySerializedName(std::string_view serializedName) {
            auto itr = _registeredCharacteristics.find(std::string(serializedName));
            if (itr != _registeredCharacteristics.end()) {
                return itr->second;
            }

            // do we want the fallback
            // GlobalNamespace::BeatmapCharacteristic characteristic{};
            // if (GlobalNamespace::BeatmapCharacteristicExtensions::BeatmapCharacteristicFromSerializedName(
            //         StringW(serializedName), by_ref(characteristic))) {
            //     return CharacteristicInfo(characteristic);
            // }

            DEBUG("Failed to find characteristic with serializedName: {}", serializedName);
            return std::nullopt;
        }

        std::optional<CharacteristicInfo> GetCharacteristic(GlobalNamespace::BeatmapCharacteristic characteristic) {
            for (auto const& [name, info] : _registeredCharacteristics) {
                if (info.sortingOrder == (int)characteristic) {
                    return info;
                }
            }

            return std::nullopt;
        }

        void RegisterCustomCharacteristic(CharacteristicInfo characteristic) {
            auto itr = _registeredCharacteristics.find(characteristic.serializedName);
            if (itr != _registeredCharacteristics.end()) {
                WARNING("Characteristic '{}' was registered more than once! not registering again", characteristic.serializedName);
                return;
            }

            _registeredCharacteristics.emplace(characteristic.serializedName, characteristic);
            _characteristicsUpdatedEvent.invoke(characteristic, CharacteristicEventKind::Registered);
        }

        void UnregisterCustomCharacteristic(CharacteristicInfo characteristic) {
            auto itr = _registeredCharacteristics.find(characteristic.serializedName);
            if (itr == _registeredCharacteristics.end()) {
                WARNING("Characteristic '{}' was unregistered more than once! not unregistering again", characteristic.serializedName);
                return;
            }

            auto info = itr->second;
            _registeredCharacteristics.erase(itr);
            _characteristicsUpdatedEvent.invoke(info, CharacteristicEventKind::Unregistered);
        }

        std::vector<CharacteristicInfo> GetRegisteredCharacteristics() {
            std::vector<CharacteristicInfo> result;
            result.reserve(_registeredCharacteristics.size());
            for (auto const& [name, info] : _registeredCharacteristics) {
                result.push_back(info);
            }
            return result;
        }

        unordered_event_callback<CharacteristicInfo, Characteristics::CharacteristicEventKind>& GetCharacteristicsUpdatedEvent() {
            return _characteristicsUpdatedEvent;
        }

        CharacteristicInfo CreateCharacteristic(UnityEngine::Sprite* icon, StringW characteristicName, StringW hintText, StringW serializedName, StringW compoundIdPartName, bool requires360Movement, bool containsRotationEvents, int sortingOrder) {
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

          return CharacteristicInfo(characteristic);
        }


        CharacteristicInfo::CharacteristicInfo(GlobalNamespace::BeatmapCharacteristic characteristic)
            : serializedName(static_cast<std::string>(GlobalNamespace::BeatmapCharacteristicExtensions::SerializedName(characteristic))),
            compoundIdPartName(static_cast<std::string>(GlobalNamespace::BeatmapCharacteristicExtensions::CompoundIdPartName(characteristic))),
            characteristicNameLocalizationKey(static_cast<std::string>(GlobalNamespace::BeatmapCharacteristicExtensions::NameLocalizationKey(characteristic))),
            descriptionLocalizationKey(static_cast<std::string>(GlobalNamespace::BeatmapCharacteristicExtensions::HintLocalizationKey(characteristic))),
            sortingOrder(static_cast<int>(characteristic)),
            requires360Movement(GlobalNamespace::BeatmapCharacteristicExtensions::Requires360Movement(characteristic)),
            containsRotationEvents(GlobalNamespace::BeatmapCharacteristicExtensions::ContainsRotationEvents(characteristic)) {}

        CharacteristicInfo::CharacteristicInfo(GlobalNamespace::BeatmapCharacteristicSO* characteristic)
            : serializedName(characteristic->serializedName),
            compoundIdPartName(characteristic->compoundIdPartName),
            characteristicNameLocalizationKey(characteristic->characteristicNameLocalizationKey),
            descriptionLocalizationKey(characteristic->descriptionLocalizationKey),
            sortingOrder(characteristic->sortingOrder),
            requires360Movement(characteristic->requires360Movement),
            containsRotationEvents(characteristic->containsRotationEvents),
            icon(characteristic->icon.ptr()),
            characteristicSO(characteristic) {}

        CharacteristicInfo::CharacteristicInfo(UnityEngine::Sprite* icon, std::string characteristicNameLocalizationKey, std::string descriptionLocalizationKey, std::string serializedName, std::string compoundIdPartName, bool requires360Movement, bool containsRotationEvents, int sortingOrder)
            : serializedName(std::move(serializedName)),
            compoundIdPartName(std::move(compoundIdPartName)),
            characteristicNameLocalizationKey(std::move(characteristicNameLocalizationKey)),
            descriptionLocalizationKey(std::move(descriptionLocalizationKey)),
            sortingOrder(sortingOrder),
            requires360Movement(requires360Movement),
            containsRotationEvents(containsRotationEvents),
            icon(icon) {
            if (icon) icon->texture->wrapMode = UnityEngine::TextureWrapMode::Clamp;

            auto so = UnityEngine::ScriptableObject::CreateInstance<GlobalNamespace::BeatmapCharacteristicSO*>();
            so->hideFlags = so->hideFlags | UnityEngine::HideFlags::DontUnloadUnusedAsset;
            so->_icon = icon;
            so->_descriptionLocalizationKey = this->descriptionLocalizationKey;
            so->_serializedName = this->serializedName;
            so->_characteristicNameLocalizationKey = this->characteristicNameLocalizationKey;
            so->_compoundIdPartName = this->compoundIdPartName;
            so->_requires360Movement = this->requires360Movement;
            so->_containsRotationEvents = this->containsRotationEvents;
            so->_sortingOrder = this->sortingOrder;
            characteristicSO = so;
        }
    }

    namespace Loading {
        static unordered_event_callback<std::span<SongCore::SongLoader::CustomBeatmapLevel* const>> _songsLoadedEvent;
        static unordered_event_callback<> _songsWillRefreshEvent;
        static unordered_event_callback<SongCore::SongLoader::CustomBeatmapLevelsRepository*> _customLevelPacksWillRefreshEvent;
        static unordered_event_callback<SongCore::SongLoader::CustomBeatmapLevelsRepository*> _customLevelPacksRefreshedEvent;
        static unordered_event_callback<SongCore::SongLoader::CustomBeatmapLevel*> _songWillBeDeletedEvent;
        static unordered_event_callback<> _songDeletedEvent;

        std::shared_future<void> RefreshSongs(bool fullRefresh) {
            auto instance = SongLoader::RuntimeSongLoader::get_instance();
            if (!instance) return std::future<void>();
            return instance->RefreshSongs(fullRefresh);
        }

        void RefreshLevelPacks() {
            auto instance = SongLoader::RuntimeSongLoader::get_instance();
            if (!instance) return;
            return instance->RefreshLevelPacks();
        }

        std::future<void> DeleteSong(std::filesystem::path const& levelPath) {
            auto instance = SongLoader::RuntimeSongLoader::get_instance();
            if (!instance) return std::future<void>();
            return instance->DeleteSong(levelPath);
        }

        std::future<void> DeleteSong(SongCore::SongLoader::CustomBeatmapLevel* beatmapLevel) {
            auto instance = SongLoader::RuntimeSongLoader::get_instance();
            if (!instance) return std::future<void>();
            return instance->DeleteSong(beatmapLevel);
        }

        unordered_event_callback<std::span<SongCore::SongLoader::CustomBeatmapLevel* const>>& GetSongsLoadedEvent() {
            return _songsLoadedEvent;
        }

        unordered_event_callback<>& GetSongsWillRefreshEvent() {
            return _songsWillRefreshEvent;
        }

        unordered_event_callback<SongCore::SongLoader::CustomBeatmapLevelsRepository*>& GetCustomLevelPacksWillRefreshEvent() {
            return _customLevelPacksWillRefreshEvent;
        }

        unordered_event_callback<SongCore::SongLoader::CustomBeatmapLevelsRepository*>& GetCustomLevelPacksRefreshedEvent() {
            return _customLevelPacksRefreshedEvent;
        }

        unordered_event_callback<SongCore::SongLoader::CustomBeatmapLevel*>& GetSongWillBeDeletedEvent() {
            return _songWillBeDeletedEvent;
        }

        unordered_event_callback<>& GetSongDeletedEvent() {
            return _songDeletedEvent;
        }

        std::filesystem::path GetPreferredCustomLevelPath() {
            if (config.RootCustomLevelPaths.empty()) return "/sdcard/ModData/com.beatgames.beatsaber/Mods/SongCore/CustomLevels";
            return config.RootCustomLevelPaths.front();
        }

        std::span<std::filesystem::path const> GetRootCustomLevelPaths() {
            return config.RootCustomLevelPaths;
        }

        std::filesystem::path GetPreferredCustomWIPLevelPath() {
            if (config.RootCustomWIPLevelPaths.empty()) return "/sdcard/ModData/com.beatgames.beatsaber/Mods/SongCore/CustomWIPLevels";
            return config.RootCustomWIPLevelPaths.front();
        }

        std::span<std::filesystem::path const> GetRootCustomWIPLevelPaths() {
            return config.RootCustomWIPLevelPaths;
        }

        void AddLevelPath(std::filesystem::path const& path, bool wipPath) {
            auto& targetPaths = wipPath ? config.RootCustomWIPLevelPaths : config.RootCustomLevelPaths;
            auto itr = std::find(targetPaths.begin(), targetPaths.end(), path);
            if (itr == targetPaths.end()) {
                targetPaths.emplace_back(path);
                SaveConfig();
            } else {
                INFO("Path {} was already in the target collection, not adding again", path.string());
            }
        }

        void RemoveLevelPath(std::filesystem::path const& path, bool wipPath) {
            auto& targetPaths = wipPath ? config.RootCustomWIPLevelPaths : config.RootCustomLevelPaths;
            auto itr = std::find(targetPaths.begin(), targetPaths.end(), path);
            if (itr != targetPaths.end()) {
                targetPaths.erase(itr);
                SaveConfig();
            } else {
                INFO("Path {} wasn't in the target collection, nothing will happen", path.string());
            }
        }

        bool AreSongsRefreshing() {
            auto instance = SongLoader::RuntimeSongLoader::get_instance();
            if (!instance) return false;
            return instance->AreSongsRefreshing;
        }

        bool AreSongsLoaded() {
            auto instance = SongLoader::RuntimeSongLoader::get_instance();
            if (!instance) return false;
            return instance->AreSongsLoaded;
        }

        float LoadProgress() {
            auto instance = SongLoader::RuntimeSongLoader::get_instance();
            if (!instance) return 0.0f;
            return instance->Progress;
        }

        std::span<SongCore::SongLoader::CustomBeatmapLevel* const> GetAllLevels() {
            static std::array<SongCore::SongLoader::CustomBeatmapLevel*, 0x0> emptyArray;
            auto instance = SongLoader::RuntimeSongLoader::get_instance();
            if (!instance) return emptyArray;
            return instance->AllLevels;
        }

        SongLoader::CustomLevelPack* GetCustomLevelPack() {
            auto instance = SongLoader::RuntimeSongLoader::get_instance();
            if (!instance) return nullptr;
            return instance->CustomLevelPack;
        }

        SongLoader::CustomLevelPack* GetCustomWIPLevelPack() {
            auto instance = SongLoader::RuntimeSongLoader::get_instance();
            if (!instance) return nullptr;
            return instance->CustomWIPLevelPack;
        }

        SongLoader::CustomBeatmapLevelsRepository* GetCustomBeatmapLevelsRepository() {
            auto instance = SongLoader::RuntimeSongLoader::get_instance();
            if (!instance) return nullptr;
            return instance->CustomBeatmapLevelsRepository;
        }

        SongCore::SongLoader::CustomBeatmapLevel* GetLevelByPath(std::filesystem::path const& levelPath) {
            auto instance = SongLoader::RuntimeSongLoader::get_instance();
            if (!instance) return nullptr;
            return instance->GetLevelByPath(levelPath);
        }

        SongCore::SongLoader::CustomBeatmapLevel* GetLevelByLevelID(std::string_view levelID) {
            auto instance = SongLoader::RuntimeSongLoader::get_instance();
            if (!instance) return nullptr;
            return instance->GetLevelByLevelID(levelID);
        }

        SongCore::SongLoader::CustomBeatmapLevel* GetLevelByHash(std::string_view hash) {
            auto instance = SongLoader::RuntimeSongLoader::get_instance();
            if (!instance) return nullptr;
            return instance->GetLevelByHash(hash);
        }

        SongCore::SongLoader::CustomBeatmapLevel* GetLevelByFunction(std::function<bool(SongCore::SongLoader::CustomBeatmapLevel*)> searchFunction) {
            auto instance = SongLoader::RuntimeSongLoader::get_instance();
            if (!instance) return nullptr;
            return instance->GetLevelByFunction(searchFunction);
        }
    }

    namespace LevelSelect {
        unordered_event_callback<LevelWasSelectedEventArgs const&> _levelWasSelectedEvent;
        unordered_event_callback<LevelWasSelectedEventArgs const&>& GetLevelWasSelectedEvent() {
            return _levelWasSelectedEvent;
        }
    }
}
