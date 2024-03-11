#include "SongCore.hpp"
#include "SongLoader/SongLoader.hpp"
#include "UI/ProgressBar.hpp"
#include "_config.h"
#include "config.hpp"
#include "main.hpp"
#include "lapiz/shared/zenject/Zenjector.hpp"
#include "bsml/shared/BSML.hpp"
#include "custom-types/shared/register.hpp"
#include "logging.hpp"
#include "hooking.hpp"
#include "assets.hpp"
#include "Zenject/FromBinderNonGeneric.hpp"
#include "Zenject/ConcreteBinderGeneric_1.hpp"
#include "lapiz/shared/utilities/ZenjectExtensions.hpp"
#include "lapiz/shared/AttributeRegistration.hpp"

#include "Capabilities.hpp"
#include "Characteristics.hpp"
#include "include/UI/ProgressBar.hpp"
#include "config.hpp"
#include <filesystem>

void RegisterDefaultCharacteristics();

static modloader::ModInfo modInfo = {MOD_ID, VERSION, 0}; // Stores the ID and version of our mod, and is sent to the modloader upon startup

Logger& getLogger() {
    static auto logger = new Logger(modInfo, LoggerOptions(false, true));
    return *logger;
}

// Loads the config from disk using our modInfo, then returns it for use
// other config tools such as config-utils don't use this config, so it can be removed if those are in use
Configuration& getConfig() {
    static Configuration config(modInfo);
    return config;
}

// Called at the early stages of game loading
SONGCORE_EXPORT_FUNC void setup(CModInfo* info) {
    info->id = MOD_ID;
    info->version = VERSION;

    getConfig().Load();
    INFO("Completed setup!");
}

MAKE_HOOK(abort_hook, nullptr, void) {
    getLogger().info("abort called");
    getLogger().Backtrace(40);

    abort_hook();
}

// Called later on in the game loading - a good time to install function hooks
SONGCORE_EXPORT_FUNC void late_load() {
    il2cpp_functions::Init();
    auto z = Lapiz::Zenject::Zenjector::Get();
    BSML::Init();
    custom_types::Register::AutoRegister();
    SongCore::Hooking::InstallHooks(getLogger());
    Lapiz::Attributes::AutoRegister();

    auto libc = dlopen("libc.so", RTLD_NOW);
    auto abrt = dlsym(libc, "abort");

    INSTALL_HOOK_DIRECT(getLogger(), abort_hook, abrt);

    if (!LoadConfig()) SaveConfig();
    if (!config.PreferredCustomLevelPath.empty() && !std::filesystem::exists(config.PreferredCustomLevelPath)) std::filesystem::create_directories(config.PreferredCustomLevelPath);
    if (!config.PreferredCustomWIPLevelPath.empty() && !std::filesystem::exists(config.PreferredCustomWIPLevelPath)) std::filesystem::create_directory(config.PreferredCustomWIPLevelPath);

    z->Install(Lapiz::Zenject::Location::App, [](::Zenject::DiContainer* container){
        INFO("Installing RSL to location App from SongCore");
        container->BindInterfacesAndSelfTo<SongCore::Capabilities*>()->AsSingle()->NonLazy();
        container->BindInterfacesAndSelfTo<SongCore::Characteristics*>()->AsSingle()->NonLazy();
        Lapiz::Zenject::ZenjectExtensions::FromNewComponentOnNewGameObject(container->Bind<SongCore::SongLoader::RuntimeSongLoader*>())->AsSingle()->NonLazy();
    });

    // z->Install(Lapiz::Zenject::Location::Menu, [](::Zenject::DiContainer* container) {
    //     container->BindInterfacesAndSelfTo<SongCore::UI::ProgressBar*>()->AsSingle();
    // });

    RegisterDefaultCharacteristics();

    INFO("SongCore loaded and initialized.");
}

void RegisterDefaultCharacteristics() {
    static SafePtrUnity<GlobalNamespace::BeatmapCharacteristicSO> missingCharacteristic = SongCore::API::Characteristics::CreateCharacteristic(
        BSML::Lite::ArrayToSprite(Assets::Resources::MissingChar_png),
        "Missing Characteristic",
        "Missing Characteristic",
        "MissingCharacteristic",
        "MissingCharacteristic",
        false,
        false,
        1000
    );

    static SafePtrUnity<GlobalNamespace::BeatmapCharacteristicSO> lightshow = SongCore::API::Characteristics::CreateCharacteristic(
        BSML::Lite::ArrayToSprite(Assets::Resources::Lightshow_png),
        "Lightshow",
        "Lightshow",
        "Lightshow",
        "Lightshow",
        false,
        false,
        100
    );

    static SafePtrUnity<GlobalNamespace::BeatmapCharacteristicSO> lawless = SongCore::API::Characteristics::CreateCharacteristic(
        BSML::Lite::ArrayToSprite(Assets::Resources::Lawless_png),
        "Lawless",
        "Lawless - Anything Goes",
        "Lawless",
        "Lawless",
        false,
        false,
        101
    );

    SongCore::API::Characteristics::RegisterCustomCharacteristic(missingCharacteristic.ptr());
    SongCore::API::Characteristics::RegisterCustomCharacteristic(lightshow.ptr());
    SongCore::API::Characteristics::RegisterCustomCharacteristic(lawless.ptr());
}
