#include "hooking.hpp"
#include "SongCore.hpp"

#include "GlobalNamespace/BeatmapCharacteristicExtensions.hpp"

// 1.45 uses enum values in memory, but still stores characteristic names in
// PlayerData.dat. The game's serializer maps every non-built-in value to
// Standard, collapsing custom scores into the Standard score on the next load.
MAKE_AUTO_HOOK_MATCH(
    BeatmapCharacteristicExtensions_SerializedName,
    &GlobalNamespace::BeatmapCharacteristicExtensions::SerializedName,
    StringW, GlobalNamespace::BeatmapCharacteristic characteristic
) {
    // Use the registry populated in late_load: player data can load before
    // the scene's SongCore::Characteristics service has been initialized.
    if (auto info = SongCore::API::Characteristics::GetCharacteristic(characteristic)) {
        return StringW(info->serializedName);
    }

    // Fallback to the built-in behavior for built-in characteristics.
    return BeatmapCharacteristicExtensions_SerializedName(characteristic);
}

MAKE_AUTO_HOOK_MATCH(
    BeatmapCharacteristicExtensions_BeatmapCharacteristicFromSerializedName,
    &GlobalNamespace::BeatmapCharacteristicExtensions::BeatmapCharacteristicFromSerializedName,
    bool, StringW name, by_ref<GlobalNamespace::BeatmapCharacteristic> characteristic
) {
    // Check if the characteristic is a built-in one first, to avoid overwriting it with a custom one.
    if (BeatmapCharacteristicExtensions_BeatmapCharacteristicFromSerializedName(name, characteristic)) {
        return true;
    }
    if (name) {
        // Use the registry populated in late_load: player data can load before
        // the scene's SongCore::Characteristics service has been initialized.
        if (auto info = SongCore::API::Characteristics::GetCharacteristicBySerializedName(static_cast<std::string>(name))) {
            *characteristic = GlobalNamespace::BeatmapCharacteristic(info->sortingOrder);
            return true;
        }
    }
    return false;
}
