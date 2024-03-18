#include "UI/SongLoaderWarning.hpp"

#include "bsml/shared/BSML.hpp"
#include "bsml/shared/Helpers/delegates.hpp"
#include "HMUI/ViewController.hpp"
#include "config.hpp"

DEFINE_TYPE(SongCore::UI, SongLoaderWarning);

namespace SongCore::UI {
    void SongLoaderWarning::ctor(GlobalNamespace::MainMenuViewController* mainMenuViewController) {
        INVOKE_CTOR();
        _mainMenuViewController = mainMenuViewController;
    }

    void SongLoaderWarning::Initialize() {
        _mainMenuViewControllerDidActivateEvent = BSML::MakeDelegate<HMUI::ViewController::DidActivateDelegate*>(
            std::function<void(bool, bool, bool)>(
                std::bind(&SongLoaderWarning::MainMenuViewControllerDidShow, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
            )
        );

        _mainMenuViewController->add_didActivateEvent(_mainMenuViewControllerDidActivateEvent);
    }

    void SongLoaderWarning::MainMenuViewControllerDidShow(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
        _mainMenuViewController->remove_didActivateEvent(_mainMenuViewControllerDidActivateEvent);
        BSML::parse_and_construct("<modal id='_warningModal' size-delta-x='70' size-delta-y='30' click-off-closes='true'><vertical><text text='You have both songcore and songloader installed' align='Center'/><text text='These mods conflict.' align='Center'/><text word-wrapping='true' text='This means songcore will not do anything.' align='Center'/><text text='Uninstall songloader to use songcore.' align='Center'/><horizontal><button text='Ok' on-click='_warningModal#Hide'/><button on-click='DontShowAgain' text='Don&apos;t show again'/></horizontal></vertical></modal>", _mainMenuViewController->transform, this);
        _warningModal->Show();
    }

    void SongLoaderWarning::DontShowAgain() {
        config.dontShowSongloaderWarningAgain = true;
        SaveConfig();
        _warningModal->Hide();
    }
}
