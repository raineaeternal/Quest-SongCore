#include "config.hpp"
#include "logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"
#include <string>

Config config;

Configuration& get_config() {
    static Configuration config({MOD_ID, VERSION, 0});
    config.Load();
    return config;
}

#define SET(name) doc.AddMember(#name, config.name, allocator)

void SaveConfig() {
    INFO("Saving Config...");
    rapidjson::Document& doc = get_config().config;
    doc.RemoveAllMembers();
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    doc.AddMember("PreferredCustomLevelPath", rapidjson::StringRef(config.PreferredCustomLevelPath.string()), allocator);
    rapidjson::Value rootCustomLevelPaths;
    rootCustomLevelPaths.SetArray();
    for (auto& path : config.RootCustomLevelPaths) {
        rootCustomLevelPaths.PushBack(rapidjson::StringRef(path.string()), allocator);
    }
    doc.AddMember("RootCustomLevelPaths", rootCustomLevelPaths, allocator);

    doc.AddMember("PreferredCustomWIPLevelPath", rapidjson::StringRef(config.PreferredCustomWIPLevelPath.string()), allocator);
    rapidjson::Value rootCustomWIPLevelPaths;
    rootCustomWIPLevelPaths.SetArray();
    for (auto& path : config.RootCustomWIPLevelPaths) {
        rootCustomWIPLevelPaths.PushBack(rapidjson::StringRef(path.string()), allocator);
    }
    doc.AddMember("RootCustomWIPLevelPaths", rootCustomWIPLevelPaths, allocator);

    get_config().Write();
    INFO("Config Saved!");
}

#define GET(name) \
if (auto name##_itr = doc.FindMember(#name); name##_itr != doc.MemberEnd()) {   \
    config.name = name##_itr->value.Get<decltype(config.name)>();                \
} else {                                                                        \
    foundEverything = false;                                                    \
}

bool LoadConfig() {
    INFO("Loading Config...");
    bool foundEverything = true;
    rapidjson::Document& doc = get_config().config;

    if (auto PreferredCustomLevelPathItr = doc.FindMember("PreferredCustomLevelPath"); PreferredCustomLevelPathItr != doc.MemberEnd()) {
        config.PreferredCustomLevelPath = PreferredCustomLevelPathItr->value.Get<std::string>();
    } else {
        foundEverything = false;
    }

    auto RootCustomLevelPathsItr = doc.FindMember("RootCustomLevelPaths");
    if (RootCustomLevelPathsItr != doc.MemberEnd() && RootCustomLevelPathsItr->value.IsArray()) {
        config.RootCustomLevelPaths.clear();
        auto arr = RootCustomLevelPathsItr->value.GetArray();
        for (auto itr = arr.Begin(); itr != arr.End(); itr++) {
            config.RootCustomLevelPaths.emplace_back(itr->Get<std::string>());
        }
    } else {
        foundEverything = false;
    }

    if (auto PreferredCustomWIPLevelPathItr = doc.FindMember("PreferredCustomWIPLevelPath"); PreferredCustomWIPLevelPathItr != doc.MemberEnd()) {
        config.PreferredCustomWIPLevelPath = PreferredCustomWIPLevelPathItr->value.Get<std::string>();
    } else {
        foundEverything = false;
    }

    auto RootCustomWIPLevelPathsItr = doc.FindMember("RootCustomWIPLevelPaths");
    if (RootCustomWIPLevelPathsItr != doc.MemberEnd() && RootCustomWIPLevelPathsItr->value.IsArray()) {
        config.RootCustomWIPLevelPaths.clear();
        auto arr = RootCustomWIPLevelPathsItr->value.GetArray();
        for (auto itr = arr.Begin(); itr != arr.End(); itr++) {
            config.RootCustomWIPLevelPaths.emplace_back(itr->Get<std::string>());
        }
    } else {
        foundEverything = false;
    }

    if (foundEverything)
        INFO("Config Loaded!");

    return foundEverything;
}
