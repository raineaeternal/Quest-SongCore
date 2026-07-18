#include "config.hpp"
#include "logging.hpp"
#include "beatsaber-hook/shared/rapidjson.hpp"
#include "beatsaber-hook/shared/utils.hpp"

Config config;
rapidjson::Document doc{rapidjson::kNullType};

#define SET(name) doc.AddMember(#name, config.name, allocator)

void SaveConfig() {
    INFO("Saving Config...");
    doc.RemoveAllMembers();
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    SET(customSongNoteColors);
    SET(customSongObstacleColors);
    SET(customSongEnvironmentColors);
    SET(disableOneSaberOverride);
    SET(dontShowSongloaderWarningAgain);

    rapidjson::Value rootCustomLevelPaths;
    rootCustomLevelPaths.SetArray();
    for (auto& path : config.RootCustomLevelPaths) {
        auto pathString = path.string();
        rootCustomLevelPaths.PushBack(rapidjson::Value(pathString.c_str(), pathString.length(), allocator), allocator);
    }
    doc.AddMember("RootCustomLevelPaths", rootCustomLevelPaths, allocator);

    rapidjson::Value rootCustomWIPLevelPaths;
    rootCustomWIPLevelPaths.SetArray();
    for (auto& path : config.RootCustomWIPLevelPaths) {
        auto pathString = path.string();
        rootCustomWIPLevelPaths.PushBack(rapidjson::Value(pathString.c_str(), pathString.length(), allocator), allocator);
    }
    doc.AddMember("RootCustomWIPLevelPaths", rootCustomWIPLevelPaths, allocator);

    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter writer(buf);
    doc.Accept(writer);
    writefile(get_config_path(MOD_ID), buf.GetString());

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
    if (doc.IsNull()) {
        auto path = get_config_path(MOD_ID);
        if (!fileexists(path)) {
            writefile(path, "{}");
            doc.SetObject();
        } else {
            doc.Parse(readfile(path));
            if (doc.HasParseError() || !doc.IsObject()) {
                WARNING("Config was invalid! Clearing.");
                doc.SetObject();
            }
        }
    }

    bool foundEverything = true;

    GET(customSongNoteColors);
    GET(customSongObstacleColors);
    GET(customSongEnvironmentColors);
    GET(disableOneSaberOverride);
    GET(dontShowSongloaderWarningAgain);

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
