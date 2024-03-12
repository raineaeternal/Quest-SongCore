#pragma once

#include <string>
#include <string_view>
#include <filesystem>
#include <vector>
#include <mutex>
#include <set>

#include "custom-types/shared/macros.hpp"

#include "../CustomJSONData.hpp"
#include "SongCoreCustomLevelPack.hpp"
#include "SongCoreCustomBeatmapLevelPackCollection.hpp"

#include "System/Collections/Concurrent/ConcurrentDictionary_2.hpp"
#include "System/Collections/Generic/List_1.hpp"

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/CustomBeatmapLevelCollection.hpp"
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/BeatmapLevelData.hpp"
#include "GlobalNamespace/BeatmapDataLoader.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/CustomLevelLoader.hpp"
#include "GlobalNamespace/CachedMediaAsyncLoader.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollection.hpp"
#include "GlobalNamespace/AdditionalContentModel.hpp"
#include "GlobalNamespace/IAdditionalContentModel.hpp"

#include "UnityEngine/MonoBehaviour.hpp"
#include "Zenject/IInitializable.hpp"
#include "lapiz/shared/macros.hpp"

namespace SongCore::SongLoader {
    using SongDict = ::System::Collections::Concurrent::ConcurrentDictionary_2<StringW, ::GlobalNamespace::CustomPreviewBeatmapLevel*>;
}

DECLARE_CLASS_CODEGEN(SongCore::SongLoader, RuntimeSongLoader, UnityEngine::MonoBehaviour,
    DECLARE_CTOR(ctor);

    DECLARE_INSTANCE_METHOD(void, Awake);

    DECLARE_INJECT_METHOD(void, Inject, GlobalNamespace::CustomLevelLoader* customLevelLoader, GlobalNamespace::BeatmapLevelsModel* _beatmapLevelsModel, GlobalNamespace::CachedMediaAsyncLoader* cachedMediaAsyncLoader, GlobalNamespace::BeatmapCharacteristicCollection* beatmapCharacteristicCollection, GlobalNamespace::IAdditionalContentModel* additionalContentModel);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::CustomLevelLoader*, _customLevelLoader);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::BeatmapLevelsModel*, _beatmapLevelsModel);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::CachedMediaAsyncLoader*, _cachedMediaAsyncLoader);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::BeatmapCharacteristicCollection*, _beatmapCharacteristicCollection);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::AdditionalContentModel*, _additionalContentModel);

    DECLARE_INSTANCE_FIELD_PRIVATE(SongCoreCustomLevelPack*, _customLevelPack);
    DECLARE_INSTANCE_FIELD_PRIVATE(SongCoreCustomLevelPack*, _customWIPLevelPack);
    DECLARE_INSTANCE_FIELD_PRIVATE(SongCoreCustomBeatmapLevelPackCollection*, _customLevelPackCollection);

    DECLARE_INSTANCE_FIELD_PRIVATE(int, _songCount);

    DECLARE_INSTANCE_FIELD_PRIVATE(SongDict*, _customLevels);
    DECLARE_INSTANCE_FIELD_PRIVATE(SongDict*, _customWIPLevels);

    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::BeatmapDataLoader*, _beatmapDataLoader);

    std::filesystem::path _songPath, _wipSongPath;
public:
    static inline const std::string CUSTOM_LEVEL_PREFIX_ID = "custom_level_";
    static inline const std::string CUSTOM_LEVEL_PACK_PREFIX_ID = "custom_levelPack_";

    /// @brief Returns an instance of the songloader
    RuntimeSongLoader* get_instance() { return _instance; };
    RuntimeSongLoader* _instance = nullptr;

    /// @brief Returns the preferred song path.
    std::filesystem::path get_SongPath() const;
    __declspec(property(get = get_SongPath)) std::filesystem::path SongPath;

    /// @brief Returns the current WIP song path.
    std::filesystem::path get_WIPSongPath() const;
    __declspec(property(get = get_WIPSongPath)) std::filesystem::path WIPSongPath;

    __declspec(property(get = get_songs)) ListW<GlobalNamespace::BeatmapData*> _songs[];

    /// @brief Returns the current song count
    int get_songCount() { return _songCount; };
    __declspec(property(get = get_songCount)) int songCount;

    /// @brief Returns the custom level dict
    SongDict* get_customLevels() { return _customLevels; };
    __declspec(property(get = get_customLevels)) SongLoader::SongDict* CustomLevels;

    /// @brief Returns the custom wip level dict
    SongDict* get_customWIPLevels() { return _customWIPLevels; };
    __declspec(property(get = get_customWIPLevels)) SongLoader::SongDict* CustomWIPLevels;

    /// @brief returns the levelpack songcore makes
    SongCoreCustomLevelPack* get_CustomLevelPack() const { return _customLevelPack; }
    __declspec(property(get = get_CustomLevelPack)) SongCoreCustomLevelPack* CustomLevelPack;

    /// @brief returns the wip levelpack songcore makes
    SongCoreCustomLevelPack* get_CustomWIPLevelPack() const { return _customWIPLevelPack; }
    __declspec(property(get = get_CustomWIPLevelPack)) SongCoreCustomLevelPack* CustomWIPLevelPack;

    /// @brief returns the beatmaplevelpackcollection songcore makes
    SongCoreCustomBeatmapLevelPackCollection* get_CustomBeatmapLevelPackCollection() const { return _customLevelPackCollection; }
    __declspec(property(get = get_CustomBeatmapLevelPackCollection)) SongCoreCustomBeatmapLevelPackCollection* CustomBeatmapLevelPackCollection;

    /// @brief refreshes the loaded songs, loading any new ones
    /// @param fullRefresh whether to reload every song or only new ones
    std::shared_future<void> RefreshSongs(bool fullRefresh);

    __declspec(property(get=get_AreSongsRefreshing)) bool AreSongsRefreshing;
    bool get_AreSongsRefreshing() const { return _currentlyLoadingFuture.valid(); }

    __declspec(property(get=get_AreSongsLoaded)) bool AreSongsLoaded;
    bool get_AreSongsLoaded() const { return _areSongsLoaded; }

    __declspec(property(get=get_Progress)) float Progress;
    /// @brief gets the current song loading progress
    float get_Progress() const { return _loadProgress; }

    __declspec(property(get=get_AllLevels)) std::span<GlobalNamespace::CustomPreviewBeatmapLevel* const> AllLevels;
    /// @brief provides access into a span of all loaded levels
    std::span<GlobalNamespace::CustomPreviewBeatmapLevel* const> get_AllLevels() const { return _allLoadedLevels; };

    /// @brief event invoked when songs will be refreshed
    UnorderedEventCallback<> SongsWillRefresh;

    /// @brief event invoked after song loading has completed, ran on main thread. the provided span is a readonly reference to all levels
    UnorderedEventCallback<std::span<GlobalNamespace::CustomPreviewBeatmapLevel* const>> LevelsLoaded;

    /// @brief event invoked before the beatmaplevelsmodel is updated with the new collection
    UnorderedEventCallback<SongCoreCustomBeatmapLevelPackCollection*> CustomLevelPacksWillRefresh;

    /// @brief event invoked after the beatmaplevelsmodel was updated with the new collection
    UnorderedEventCallback<SongCoreCustomBeatmapLevelPackCollection*> CustomLevelPacksRefreshed;

    private:
        /// @brief internal struct to keep track of levelpath and wip status of a song before it got loaded
        struct LevelPathAndWip {
            std::filesystem::path levelPath;
            bool isWip;

            auto operator<=>(LevelPathAndWip const& other) const {
                return levelPath <=> other.levelPath;
            }
        };

        /// @brief gets the environment info with environmentName, or default if not found
        /// @param environmentName the name to look for
        /// @param allDirections if the env was not found, use the default for alldirections or not
        /// @return environmentinfo
        GlobalNamespace::EnvironmentInfoSO* GetEnvironmentInfo(StringW environmentName, bool allDirections);

        /// @brief gets all the environment infos with environmentNames
        /// @param environmentsNames the names to look for
        /// @return environmentinfo array
        ArrayW<GlobalNamespace::EnvironmentInfoSO*> GetEnvironmentInfos(std::span<StringW const> environmentsNames);

        /// @brief constructs the color schemes from the savedata
        /// @param colorSchemeDatas the save data color schemes
        /// @return constructed color schemes
        ArrayW<GlobalNamespace::ColorScheme*> GetColorSchemes(std::span<GlobalNamespace::BeatmapLevelColorSchemeSaveData* const> colorSchemeDatas);

        /// @brief fixes up the difficulty beatmap sets from the game
        ListW<GlobalNamespace::PreviewDifficultyBeatmapSet*> GetDifficultyBeatmapSets(std::span<GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmapSet* const> difficultyBeatmapSetDatas);

        /// @brief Loads songs at given path given
        /// @param path the path to the song
        /// @param isWip is this a wip song
        /// @param saveData the level save data, for custom levels this is always a custom level info savedata
        /// @param outHash output for the hash of this level, might be unneeded though
        /// @return loaded preview beatmap level, or nullptr if failed
        GlobalNamespace::CustomPreviewBeatmapLevel* LoadCustomPreviewBeatmapLevel(std::filesystem::path levelPath, bool isWip, SongCore::CustomJSONData::CustomLevelInfoSaveData* saveData, std::string& outHash);

        void CollectLevels(std::span<const std::filesystem::path> roots, bool isWip, std::set<LevelPathAndWip>& out);

        void LoadCustomLevelsFromPaths(std::span<const std::filesystem::path> paths, bool wip = false);
        void RefreshSongs_internal(bool refreshSongs);
        void RefreshSongWorkerThread(std::mutex* levelsItrMutex, std::set<LevelPathAndWip>::const_iterator* levelsItr, std::set<LevelPathAndWip>::const_iterator* levelsEnd);
        void RefreshLevelPacks();

        SongCore::CustomJSONData::CustomLevelInfoSaveData* GetStandardSaveData(std::filesystem::path path);

        std::shared_future<void> _currentlyLoadingFuture;

        std::atomic<size_t> _loadedSongs;
        std::atomic<size_t> _totalSongs;
        std::atomic<float> _loadProgress;
        std::atomic<bool> _areSongsLoaded;
        std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> _allLoadedLevels;
)
