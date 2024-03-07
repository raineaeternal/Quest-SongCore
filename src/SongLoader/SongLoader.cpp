#include "SongLoader/SongLoader.hpp"
#include "SongLoader/SongCoreCustomLevelPack.hpp"
#include "main.hpp"
#include "shared/SongCore.hpp"
#include "logging.hpp"
#include "config.hpp"
#include "paper/shared/utfcpp/source/utf8.h"

#include "Utils/File.hpp"

#include <clocale>
#include <exception>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <string.h>
#include <string>

DEFINE_TYPE(SongCore::SongLoader, RuntimeSongLoader);

#define MAX_THREAD_COUNT 8

namespace SongCore::SongLoader {
    void RuntimeSongLoader::ctor() {
        INVOKE_BASE_CTOR(classof(UnityEngine::MonoBehaviour*));
        INVOKE_CTOR();

        _customLevelPack = SongCoreCustomLevelPack::New(fmt::format("{}CustomLevels", CUSTOM_LEVEL_PACK_PREFIX_ID), "Custom Levels", nullptr);
        _customWIPLevelPack = SongCoreCustomLevelPack::New(fmt::format("{}CustomWIPLevels", CUSTOM_LEVEL_PACK_PREFIX_ID), "Custom WIP Levels", nullptr);

        _customLevels = SongDict::New_ctor();
        _customWIPLevels = SongDict::New_ctor();
    }

    void RuntimeSongLoader::Awake() {
        RefreshSongs(true);
    }

    void RuntimeSongLoader::CollectLevels(std::span<const std::filesystem::path> roots, bool isWip, std::set<LevelPathAndWip>& out) {
        for (auto& rootPath : roots) {
            if (!std::filesystem::exists(rootPath)) {
                WARNING("Attempted to load songs from folder '{}' but it did not exist! skipping...", rootPath.string());
                continue;
            }

            auto songPaths = Utils::GetFolders(rootPath);
            for (auto& songPath : songPaths) {
                auto dataPath = songPath / "info.dat";
                if (!std::filesystem::exists(dataPath)) dataPath = songPath / "Info.dat";
                if (!std::filesystem::exists(dataPath)) {
                    WARNING("Song path '{}' had no info.dat folder! skipping...", songPath.string());
                    continue;
                }

                out.emplace(songPath, isWip);
            }
        }
    }

    std::shared_future<void> RuntimeSongLoader::RefreshSongs(bool fullRefresh) {
        if (_currentlyLoadingFuture.valid()) {
            return _currentlyLoadingFuture;
        }
        _currentlyLoadingFuture = il2cpp_utils::il2cpp_async(std::launch::async, &RuntimeSongLoader::RefreshSongs_internal, this, std::forward<bool>(fullRefresh));
        return _currentlyLoadingFuture;
    }

    void RuntimeSongLoader::RefreshSongs_internal(bool fullRefresh) {
        std::set<LevelPathAndWip> levels;

        // travel the given song paths to collect levels to load
        CollectLevels(config.RootCustomLevelPaths, false, levels);
        CollectLevels(config.RootCustomWIPLevelPaths, false, levels);


        // load songs on multiple threads
        std::mutex levelsItrMutex;
        std::set<LevelPathAndWip>::const_iterator levelsItr = levels.begin();
        std::set<LevelPathAndWip>::const_iterator levelsEnd = levels.end();

        std::vector<il2cpp_utils::il2cpp_aware_thread> workerThreads;
        workerThreads.reserve(MAX_THREAD_COUNT);

        auto worker = [this, &levelsItrMutex, &levelsItr, &levelsEnd](){

        };

        for (int i = 0; i < MAX_THREAD_COUNT; i++) {
            workerThreads.emplace_back(
                &RuntimeSongLoader::RefreshSongWorkerThread,
                this,
                &levelsItrMutex,
                &levelsItr,
                &levelsEnd
            );
        }

        for (auto& t : workerThreads) {
            t.join();
        }

        // clear current future
        _currentlyLoadingFuture = std::future<void>();
    }

    void RuntimeSongLoader::RefreshSongWorkerThread(std::mutex* levelsItrMutex, std::set<LevelPathAndWip>::const_iterator* levelsItr, std::set<LevelPathAndWip>::const_iterator* levelsEnd) {
        auto NextLevel = [](std::mutex& levelsItrMutex, std::set<LevelPathAndWip>::const_iterator& levelsItr, std::set<LevelPathAndWip>::const_iterator& levelsEnd) -> LevelPathAndWip {
            std::lock_guard<std::mutex> lock(levelsItrMutex);
            if (levelsItr != levelsEnd) {
                auto v = *levelsItr;
                levelsItr++;
                return v;
            } else {
                return {};
            }
        };

        while (levelsItr != levelsEnd) {
            auto [levelPath, isWip] = NextLevel(*levelsItrMutex, *levelsItr, *levelsEnd);

            // we got an invalid levelPath
            if (levelPath.empty()) continue;

            try {
                // TODO: get current time before loading
                // TODO: check whether this songpath lives in existing collections
                // TODO: if not, attempt loading levelinfosavedata from the song path, then load custom preview beatmap level from that
                // TODO: make sure that the custom data section is loaded by default, since it's always used anyway for things like requirements and custom diff names
                // TODO: after level loaded, add to relevant collections if not already added
                // TODO: get current time after loading, log load time
            } catch (std::exception const& e) {
                ERROR("Caught exception of type {} while loading song @ path '{}', song will be skipped!", typeid(e).name(), levelPath.string());
                ERROR("Exception what: {}", e.what());
            } catch (...) {
                ERROR("Caught exception of unknown type (current_exception typeid: {}) while loading song @ path '{}', song will be skipped!", typeid(std::current_exception()).name(), levelPath.string());
            }
        }
    }

    void RuntimeSongLoader::Update() {

    }

    std::filesystem::path RuntimeSongLoader::get_SongPath() const {
        return config.PreferredCustomLevelPath;
    }

    std::filesystem::path RuntimeSongLoader::get_WIPSongPath() const {
        return config.PreferredCustomWIPLevelPath;
    }
}
