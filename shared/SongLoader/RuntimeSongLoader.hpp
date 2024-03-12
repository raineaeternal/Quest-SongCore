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
    DECLARE_INSTANCE_METHOD(void, OnDestroy);

    DECLARE_INJECT_METHOD(void, Inject, GlobalNamespace::CustomLevelLoader* customLevelLoader, GlobalNamespace::BeatmapLevelsModel* _beatmapLevelsModel, GlobalNamespace::CachedMediaAsyncLoader* cachedMediaAsyncLoader, GlobalNamespace::BeatmapCharacteristicCollection* beatmapCharacteristicCollection, GlobalNamespace::IAdditionalContentModel* additionalContentModel);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::CustomLevelLoader*, _customLevelLoader);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::BeatmapLevelsModel*, _beatmapLevelsModel);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::CachedMediaAsyncLoader*, _cachedMediaAsyncLoader);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::BeatmapCharacteristicCollection*, _beatmapCharacteristicCollection);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::AdditionalContentModel*, _additionalContentModel);

    DECLARE_INSTANCE_FIELD_PRIVATE(SongCoreCustomLevelPack*, _customLevelPack);
    DECLARE_INSTANCE_FIELD_PRIVATE(SongCoreCustomLevelPack*, _customWIPLevelPack);
    DECLARE_INSTANCE_FIELD_PRIVATE(SongCoreCustomBeatmapLevelPackCollection*, _customLevelPackCollection);

    DECLARE_INSTANCE_FIELD_PRIVATE(SongDict*, _customLevels);
    DECLARE_INSTANCE_FIELD_PRIVATE(SongDict*, _customWIPLevels);

    public:
        static inline const std::string CUSTOM_LEVEL_PREFIX_ID = "custom_level_";
        static inline const std::string CUSTOM_LEVEL_PACK_PREFIX_ID = "custom_levelPack_";

        /// @brief refreshes the loaded songs, loading any new ones
        /// @param fullRefresh whether to reload every song or only new ones
        /// @return shared future which you may await for when the songs are finished refreshing. if you want an onFinished use the `LevelsLoaded` event
        std::shared_future<void> RefreshSongs(bool fullRefresh = false);

        /// @brief refreshes the level packs in the beatmaplevelsmodel
        void RefreshLevelPacks();

        /// @brief delete a level by providing its level path
        std::future<void> DeleteSong(std::filesystem::path const& levelPath);

        /// @brief delete a level by providing a preview beatmap level, which is used to get the level path
        std::future<void> DeleteSong(GlobalNamespace::CustomPreviewBeatmapLevel* beatmapLevel);

        /// @brief Returns the instance of the songloader for people not using zenject. only valid when the app context is valid (soft restarts & game start have times when its not valid)
        static RuntimeSongLoader* get_instance() { return _instance; };

        /// @brief Returns the preferred song path.
        std::filesystem::path get_SongPath() const;
        __declspec(property(get = get_SongPath)) std::filesystem::path SongPath;

        /// @brief Returns the current WIP song path.
        std::filesystem::path get_WIPSongPath() const;
        __declspec(property(get = get_WIPSongPath)) std::filesystem::path WIPSongPath;

        /// @brief Returns the custom level dictionary
        SongDict* get_customLevels() const { return _customLevels; };
        __declspec(property(get = get_customLevels)) SongLoader::SongDict* CustomLevels;

        /// @brief Returns the custom wip level dictionary
        SongDict* get_customWIPLevels() const { return _customWIPLevels; };
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

        /// @brief lets you check whether songs are currently being refreshed
        bool get_AreSongsRefreshing() const { return _currentlyLoadingFuture.valid(); }
        __declspec(property(get=get_AreSongsRefreshing)) bool AreSongsRefreshing;

        /// @brief lets you check whether songs loaded are valid right now
        bool get_AreSongsLoaded() const { return _areSongsLoaded; }
        __declspec(property(get=get_AreSongsLoaded)) bool AreSongsLoaded;

        /// @brief gets the current song loading progress
        float get_Progress() const { return _loadProgress; }
        __declspec(property(get=get_Progress)) float Progress;

        /// @brief provides access into a span of all loaded levels
        std::span<GlobalNamespace::CustomPreviewBeatmapLevel* const> get_AllLevels() const { return _allLoadedLevels; };
        __declspec(property(get=get_AllLevels)) std::span<GlobalNamespace::CustomPreviewBeatmapLevel* const> AllLevels;

        /// @brief event invoked when songs will be refreshed
        UnorderedEventCallback<> SongsWillRefresh;

        /// @brief event invoked after song loading has completed, ran on main thread. the provided span is a readonly reference to all levels
        UnorderedEventCallback<std::span<GlobalNamespace::CustomPreviewBeatmapLevel* const>> SongsLoaded;

        /// @brief event invoked before the beatmaplevelsmodel is updated with the new collections
        UnorderedEventCallback<SongCoreCustomBeatmapLevelPackCollection*> CustomLevelPacksWillRefresh;

        /// @brief event invoked after the beatmaplevelsmodel was updated with the new collection
        UnorderedEventCallback<SongCoreCustomBeatmapLevelPackCollection*> CustomLevelPacksRefreshed;

        /// @brief event invoked before a song is deleted so you can do last minute things you want to do with the deleted level
        UnorderedEventCallback<GlobalNamespace::CustomPreviewBeatmapLevel*> SongWillBeDeleted;

        /// @brief event invoked after a song got deleted, so you may redo certain operations
        UnorderedEventCallback<> SongDeleted;

        /// @brief gets a level by the levelpath
        /// @return nullptr if level not found
        GlobalNamespace::CustomPreviewBeatmapLevel* GetLevelByPath(std::filesystem::path const& levelPath);

        /// @brief gets a level by the levelId
        /// @return nullptr if level not found
        GlobalNamespace::CustomPreviewBeatmapLevel* GetLevelByLevelID(std::string_view levelID);

        /// @brief gets a level by the hash
        /// @return nullptr if level not found
        GlobalNamespace::CustomPreviewBeatmapLevel* GetLevelByHash(std::string_view levelID);

        /// @brief gets a level by a search function
        /// @return nullptr if level not found
        GlobalNamespace::CustomPreviewBeatmapLevel* GetLevelByFunction(std::function<bool(GlobalNamespace::CustomPreviewBeatmapLevel*)> searchFunction);

        /// @brief gets the environment info with environmentName, or default if not found
        /// @param environmentName the name to look for
        /// @param allDirections if the env was not found, use the default for alldirections or not
        /// @return environmentinfo
        GlobalNamespace::EnvironmentInfoSO* GetEnvironmentInfo(StringW environmentName, bool allDirections);

        /// @brief gets all the environment infos with environmentNames
        /// @param environmentsNames the names to look for
        /// @return environmentinfo array
        ArrayW<GlobalNamespace::EnvironmentInfoSO*> GetEnvironmentInfos(std::span<StringW const> environmentsNames);
    private:
        /// @brief internal struct to keep track of levelpath and wip status of a song before it got loaded
        struct LevelPathAndWip {
            std::filesystem::path levelPath;
            bool isWip;

            auto operator<=>(LevelPathAndWip const& other) const {
                return levelPath <=> other.levelPath;
            }
        };

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

        /// @brief collects levels from the root into the given set, and keeps the wip status
        static void CollectLevels(std::filesystem::path const& root, bool isWip, std::set<LevelPathAndWip>& out);

        /// @brief collects levels from the roots into the given set, and keeps the wip status
        static void CollectLevels(std::span<const std::filesystem::path> roots, bool isWip, std::set<LevelPathAndWip>& out);

        /// @brief method kicked of by RefreshSongs on an il2cpp async
        void RefreshSongs_internal(bool refreshSongs);

        /// @brief worker thread for loading songs from a set
        void RefreshSongWorkerThread(std::mutex* levelsItrMutex, std::set<LevelPathAndWip>::const_iterator* levelsItr, std::set<LevelPathAndWip>::const_iterator* levelsEnd);

        /// @brief internal method for deleting a song, ran through il2cpp async
        void DeleteSong_internal(std::filesystem::path levelPath);


        /// @brief gets the savedata from the path
        SongCore::CustomJSONData::CustomLevelInfoSaveData* GetStandardSaveData(std::filesystem::path path);

        /// @brief while songs are refreshing this future holds what is currently happening
        std::shared_future<void> _currentlyLoadingFuture;

        /// @brief how many songs have already been loaded
        std::atomic<size_t> _loadedSongs;
        /// @brief how many songs there are
        std::atomic<size_t> _totalSongs;
        /// @brief how far along loading the progress is
        std::atomic<float> _loadProgress;
        /// @brief are songs done loading
        std::atomic<bool> _areSongsLoaded;
        /// @brief all loaded levels
        std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> _allLoadedLevels;

        static RuntimeSongLoader* _instance;

        /// @brief invoker method for SongsWillRefresh event
        void InvokeSongsWillRefresh() const;
        /// @brief invoker method for SongsLoaded event
        void InvokeSongsLoaded(std::span<GlobalNamespace::CustomPreviewBeatmapLevel* const> levels) const;
        /// @brief invoker method for CustomLevelPacksWillRefresh event
        void InvokeCustomLevelPacksWillRefresh(SongCoreCustomBeatmapLevelPackCollection* levelPackCollection) const;
        /// @brief invoker method for CustomLevelPacksRefreshed event
        void InvokeCustomLevelPacksRefreshed(SongCoreCustomBeatmapLevelPackCollection* levelPackCollection) const;
        /// @brief invoker method for SongWillBeDeleted event
        void InvokeSongWillBeDeleted(GlobalNamespace::CustomPreviewBeatmapLevel* level) const;
        /// @brief invoker method for SongDeleted event
        void InvokeSongDeleted() const;
)
