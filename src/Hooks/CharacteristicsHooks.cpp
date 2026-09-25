#include "hooking.hpp"
#include "logging.hpp"

#include "GlobalNamespace/BeatmapCharacteristicCollection.hpp"
#include "GlobalNamespace/BeatmapCharacteristicExtensions.hpp"
#include "SongCore.hpp"

// characteristic used if none is found
#define MISSING_CHARACTERISTIC "MissingCharacteristic"

MAKE_AUTO_HOOK_MATCH(BeatmapCharacteristicCollection_GetBeatmapCharacteristicBySerializedName, &GlobalNamespace::BeatmapCharacteristicCollection::GetBeatmapCharacteristicBySerializedName, UnityW<GlobalNamespace::BeatmapCharacteristicSO>, GlobalNamespace::BeatmapCharacteristicCollection* self, StringW serializedName) {
    auto result = BeatmapCharacteristicCollection_GetBeatmapCharacteristicBySerializedName(self, serializedName);
    if (!result) {
        std::string cppSerializedName(serializedName);
        INFO("GetBeatmapCharacteristicBySerializedName failed to find characteristic with serialized name '{}'", cppSerializedName);
        if (auto info = SongCore::API::Characteristics::GetCharacteristicBySerializedName(cppSerializedName)) {
            result = info->characteristicSO.ptr();
        }
        if (!result) {
            WARNING("GetBeatmapCharacteristicBySerializedName STILL failed to find characteristic with serialized name '{}', returning '{}' instead!", cppSerializedName, MISSING_CHARACTERISTIC);
            if (auto info = SongCore::API::Characteristics::GetCharacteristicBySerializedName(MISSING_CHARACTERISTIC)) {
                result = info->characteristicSO.ptr();
            }
        }
    }

    return result;
}

// PlayerDataFileModel uses these enum conversions directly, bypassing the
// characteristic collection. Keep custom save keys distinct from Standard and
// recognize them again when loading; leave every native fallback unchanged.
MAKE_AUTO_HOOK_MATCH(BeatmapCharacteristicExtensions_SerializedName,
    static_cast<StringW (*)(GlobalNamespace::BeatmapCharacteristic)>(
        &GlobalNamespace::BeatmapCharacteristicExtensions::SerializedName),
    StringW, GlobalNamespace::BeatmapCharacteristic characteristic) {
    if (auto info = SongCore::API::Characteristics::GetCharacteristic(characteristic)) {
        return StringW(info->serializedName);
    }
    return BeatmapCharacteristicExtensions_SerializedName(characteristic);
}

MAKE_AUTO_HOOK_MATCH(BeatmapCharacteristicExtensions_FromSerializedName,
    &GlobalNamespace::BeatmapCharacteristicExtensions::BeatmapCharacteristicFromSerializedName,
    bool, StringW serializedName, by_ref<GlobalNamespace::BeatmapCharacteristic> characteristic) {
    if (BeatmapCharacteristicExtensions_FromSerializedName(serializedName, characteristic)) {
        return true;
    }
    if (serializedName) {
        if (auto info = SongCore::API::Characteristics::GetCharacteristicBySerializedName(
                std::string(serializedName))) {
            *characteristic = GlobalNamespace::BeatmapCharacteristic(info->sortingOrder);
            return true;
        }
    }
    return false;
}
