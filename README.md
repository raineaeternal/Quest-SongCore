# Quest-SongCore
A library/mod for Quest Beat Saber that handles song loading, song requirements and capabilities.

# API
In the `SongCore.hpp` you'll find various static api methods you can use to interact with songcore.

### Capabilities
This is where you can register, unregister and check capabilities that are installed. This is inteded for things maps can ask for like noodle extensions, mapping extensions, cinema. (requirements / suggestions). This is inteded to be used to let songcore know "hey this capability is installed, which means you may allow the map to be played"

Alternatively if you are using zenject, you may use the `Capabilities.hpp` header and have the api be injected through an instance songcore binds at `App`

### PlayButton
This is where you as a mod can decide whether the play button should be enabled. This is useful when something the user has configured (or a map does) conflicts with your mod, so rather than having things break you may disable the play button. This can also be used if you have to load data before the user may start a map or something similiar, as the mod eagerly checks for updates of the list of mods disabling the play buttons. You should give your mod id or some identifier when you disable / enable the play button from your mod, and when disabling you may give a reason for why you disable. If you want to give more than 1 reason you should disable with different identifiers.

Alternatively if you are using zenject, you may use the `PlayButtonInteractable.hpp` header and have the api be injected through an instance songcore binds at `App`

### Characteristics
This is where you as a mod can register custom characteristics for maps to use. Do note that when your mod is no longer installed, songcore will forget about your characteristic and scores that used it will be wiped.

Alternatively if you are using zenject, you may use the `Characteristics.hpp` header and have the api be injected through an instance songcore binds at `App`

### Loading
This is where you as a mod can access various collections and instances created by songcore. It also provides various events and checks that you may want to use in your mod.

The events that are available:
- `SongsLoadedEvent`, Invoked when refreshing is finished but before the future returned from RefreshSongs concludes
- `SongsWillRefreshEvent`, Invoked nearly first thing in the refresh songs code, but before the IsLoaded bool is set to false so you may know whether this was a cold refresh or songs were loaded before
- `CustomLevelPacksWillRefreshEvent`, Invoked before the levelpacks from the levelsrepository are added to the all loaded packs from the game
- `CustomLevelPacksRefreshedEvent`, Invoked after the levelpacks from the levelsrepository are added to the all loaded packs from the game
- `SongWillBeDeletedEvent`, Invoked before a song is deleted, with the beatmaplevel that will be deleted
- `SongDeletedEvent`, Invoked when a song was deleted

Alternatively if you are using zenject, you may use the `SongLoader/RuntimeSongLoader.hpp` header and have the api be injected through an instance songcore binds at `App`

#### Custom data
If you can get at a levelID, level hash, or just a level itself, you can get at the custom map data. SongCore CustomBeatmapLevel stores the CustomLevelInfoSaveData inside the instance, and you can `il2cpp_utils::try_cast<SongCore::SongLoader::CustomBeatmapLevel>(level)` to get the instance. Then through the `customLevel->get_standardLevelInfoSaveData()` you can access that data.

From there you can use
- `saveData->TryGetLevelDetails()` for custom data concerning the whole map
- `saveData->TryGetCharacteristic(charname)` for custom data concerning the specific characteristic
- `saveData->TryGetCharacteristicAndDifficulty(charname, diff)` for custom data concerning the specific difficulty

### LevelSelect
This is where you as a mod can access an event which is invoked when the player selects a level in the level detail view. It provides various values for you to use in case you want to do anything on level select.

Alternatively if you are using zenject, you may use the `LevelSelect.hpp` header and have the api be injected through an instance songcore binds at `Menu`
