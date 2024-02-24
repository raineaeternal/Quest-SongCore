#include "SongLoader/SongLoader.hpp"
#include "_config.h"
#include "main.hpp"
#include "lapiz/shared/zenject/Zenjector.hpp"
#include "bsml/shared/BSML.hpp"
#include "custom-types/shared/register.hpp"
#include "logging.hpp"
#include "assets.hpp"
#include "Zenject/FromBinderNonGeneric.hpp"
#include "Zenject/ConcreteBinderGeneric_1.hpp"
#include "Lapiz/shared/utilities/ZenjectExtensions.hpp"

#include "Capabilities.hpp"
#include "include/Installers/MenuInstaller.hpp"

static modloader::ModInfo modInfo = {MOD_ID, VERSION, 0}; // Stores the ID and version of our mod, and is sent to the modloader upon startup

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

// Called later on in the game loading - a good time to install function hooks
SONGCORE_EXPORT_FUNC void late_load() {
    il2cpp_functions::Init();
    auto z = Lapiz::Zenject::Zenjector::Get();
    BSML::Init();
    custom_types::Register::AutoRegister();

    z->Install(Lapiz::Zenject::Location::App, [](::Zenject::DiContainer* container){
        INFO("Installing RSL to location App from SongCore");
        container->Bind<SongCore::Capabilities*>()->ToSelf()->AsSingle()->NonLazy();
        Lapiz::Zenject::ZenjectExtensions::FromNewComponentOnNewGameObject(container->Bind<SongCore::SongLoader::RuntimeSongLoader*>())->AsSingle()->NonLazy();
    });
    z->Install<SongCore::Installers::MenuInstaller*>(Lapiz::Zenject::Location::Menu);

    INFO("SongCore loaded and initialized.");
}
