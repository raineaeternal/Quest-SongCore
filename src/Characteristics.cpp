#include "Characteristics.hpp"
#include "SongCore.hpp"

#include "System/Collections/Generic/Dictionary_2.hpp"
#include "logging.hpp"
#include <algorithm>
#include <mutex>

DEFINE_TYPE(SongCore, Characteristics);

namespace {
    SongCore::Characteristics* _instance = nullptr;
}

namespace SongCore {
    Characteristics* Characteristics::get_instance() {
        return _instance;
    }

    void Characteristics::ctor(GlobalNamespace::BeatmapCharacteristicCollection* beatmapCharacteristicCollection, GlobalNamespace::AppStaticSettingsSO* appStaticSettings) {
        _beatmapCharacteristicCollection = beatmapCharacteristicCollection;
        _appStaticSettings = appStaticSettings;

        if (!beatmapCharacteristicCollection->beatmapCharacteristics) {
            ERROR("BeatmapCharacteristicCollection is null!");
        } else {
            auto characteristics = i2c::cast<System::Collections::Generic::List_1<
                GlobalNamespace::BeatmapCharacteristic> *>(
                beatmapCharacteristicCollection->beatmapCharacteristics);
            auto characteristicsList = ListW<GlobalNamespace::BeatmapCharacteristic>(characteristics);
            _beatmapCharacteristics.reserve(characteristicsList.size());
            for (auto characteristic : characteristicsList) {
                _beatmapCharacteristics.emplace_back(characteristic);
            }
        }

        if (!beatmapCharacteristicCollection->disabledBeatmapCharacteristics) {
            ERROR("Disabled BeatmapCharacteristicCollection is null!");
        } else {
            auto disabledCharacteristics = i2c::cast<System::Collections::Generic::List_1<
                GlobalNamespace::BeatmapCharacteristic> *>(
                beatmapCharacteristicCollection->disabledBeatmapCharacteristics);
            auto disabledCharacteristicsList = ListW<GlobalNamespace::BeatmapCharacteristic>(disabledCharacteristics);
            _disabledBeatmapCharacteristics.reserve(disabledCharacteristicsList.size());
            for (auto characteristic : disabledCharacteristicsList) {
                _disabledBeatmapCharacteristics.emplace_back(characteristic);
            }
        }
    }

    void Characteristics::Initialize() {
        if (!_instance) _instance = this;

        SongCore::API::Characteristics::GetCharacteristicsUpdatedEvent() += {&Characteristics::CharacteristicsUpdated, this};
        // on initialization, add any already registered characteristics to the collection
        for (auto const& characteristic : RegisteredCharacteristics) {
            AddCharacteristicToCollection(characteristic);
        }
    }

    void Characteristics::Dispose() {
        if (_instance == this) _instance = nullptr;

        SongCore::API::Characteristics::GetCharacteristicsUpdatedEvent() -= {&Characteristics::CharacteristicsUpdated, this};
    }

    void Characteristics::RegisterCustomCharacteristic(API::Characteristics::CharacteristicInfo characteristic) {
        return SongCore::API::Characteristics::RegisterCustomCharacteristic(characteristic);
    }

    void Characteristics::UnregisterCustomCharacteristic(API::Characteristics::CharacteristicInfo characteristic) {
        return SongCore::API::Characteristics::UnregisterCustomCharacteristic(characteristic);
    }

    std::optional<API::Characteristics::CharacteristicInfo> Characteristics::GetCharacteristicBySerializedName(std::string_view serializedName) {
        auto itr = std::find_if(_beatmapCharacteristics.begin(), _beatmapCharacteristics.end(), [&](auto const& info) {
            return info.serializedName == serializedName;
        });
        if (itr != _beatmapCharacteristics.end()) return *itr;

        auto disabledItr = std::find_if(_disabledBeatmapCharacteristics.begin(), _disabledBeatmapCharacteristics.end(), [&](auto const& info) {
            return info.serializedName == serializedName;
        });
        if (disabledItr != _disabledBeatmapCharacteristics.end()) return *disabledItr;

        return std::nullopt;
    }

    API::Characteristics::CharacteristicInfo Characteristics::GetCharacteristic(GlobalNamespace::BeatmapCharacteristic characteristic) {
        auto itr = std::find_if(_beatmapCharacteristics.begin(), _beatmapCharacteristics.end(), [&](auto const& info) {
            return info.sortingOrder == (int)characteristic;
        });
        if (itr != _beatmapCharacteristics.end()) return *itr;

        auto disabledItr = std::find_if(_disabledBeatmapCharacteristics.begin(), _disabledBeatmapCharacteristics.end(), [&](auto const& info) {
            return info.sortingOrder == (int)characteristic;
        });
        if (disabledItr != _disabledBeatmapCharacteristics.end()) return *disabledItr;

        return API::Characteristics::CharacteristicInfo(characteristic);
    }

    std::vector<API::Characteristics::CharacteristicInfo> Characteristics::GetRegisteredCharacteristics() {
        return SongCore::API::Characteristics::GetRegisteredCharacteristics();
    }

    std::span<API::Characteristics::CharacteristicInfo> Characteristics::GetEnabledCharacteristics() {
        return _beatmapCharacteristics;
    }

    std::span<API::Characteristics::CharacteristicInfo> Characteristics::GetDisabledCharacteristics() {
        return _disabledBeatmapCharacteristics;
    }

    unordered_event_callback<API::Characteristics::CharacteristicInfo, SongCore::API::Characteristics::CharacteristicEventKind>& Characteristics::GetCharacteristicsUpdatedEvent() {
        return _characteristicsUpdated;
    }

    void Characteristics::CharacteristicsUpdated(API::Characteristics::CharacteristicInfo characteristic, SongCore::API::Characteristics::CharacteristicEventKind eventKind) {
        switch (eventKind) {
            using enum SongCore::API::Characteristics::CharacteristicEventKind;
            case Registered:
                AddCharacteristicToCollection(characteristic);
                break;
            case Unregistered:
                RemoveCharacteristicFromCollection(characteristic);
                break;
        }

        _characteristicsUpdated.invoke(characteristic, eventKind);
    }

    void Characteristics::AddCharacteristicToCollection(API::Characteristics::CharacteristicInfo characteristic) {
        std::lock_guard<std::mutex> lock(_collectionMutex);

        auto const& serializedName = characteristic.serializedName;
        if (characteristic.requires360Movement && !_appStaticSettings->enable360DegreeLevels) {
            auto itr = std::find_if(_disabledBeatmapCharacteristics.begin(), _disabledBeatmapCharacteristics.end(), [&](auto const& info) {
                return info.serializedName == serializedName;
            });
            if (itr == _disabledBeatmapCharacteristics.end()) {
                _disabledBeatmapCharacteristics.emplace_back(characteristic);
            }
        } else {
            auto itr = std::find_if(_beatmapCharacteristics.begin(), _beatmapCharacteristics.end(), [&](auto const& info) {
                return info.serializedName == serializedName;
            });
            if (itr == _beatmapCharacteristics.end()) {
                _beatmapCharacteristics.emplace_back(characteristic);

                // custom characteristics carry their own live BeatmapCharacteristicSO; keep the
                // game's lookup dictionary in sync so it can still be found by serialized name
                auto so = characteristic.characteristicSO.ptr();
                if (so) {
                    _beatmapCharacteristicCollection->_beatmapCharacteristicsBySerializedName->Add(StringW(serializedName), so);
                }
            }
        }
    }

    void Characteristics::RemoveCharacteristicFromCollection(API::Characteristics::CharacteristicInfo characteristic) {
        std::lock_guard<std::mutex> lock(_collectionMutex);

        auto const& serializedName = characteristic.serializedName;
        auto itr = std::find_if(_disabledBeatmapCharacteristics.begin(), _disabledBeatmapCharacteristics.end(), [&](auto const& info) {
            return info.serializedName == serializedName;
        });
        if (itr != _disabledBeatmapCharacteristics.end()) {
            _disabledBeatmapCharacteristics.erase(itr);
        }

        auto itr2 = std::find_if(_beatmapCharacteristics.begin(), _beatmapCharacteristics.end(), [&](auto const& info) {
            return info.serializedName == serializedName;
        });
        if (itr2 != _beatmapCharacteristics.end()) {
            // if the characteristic carries a live SO, it was also added to the dictionary
            if (itr2->characteristicSO.ptr()) {
                _beatmapCharacteristicCollection->_beatmapCharacteristicsBySerializedName->Remove(StringW(serializedName));
            }
            _beatmapCharacteristics.erase(itr2);
        }
    }
}
