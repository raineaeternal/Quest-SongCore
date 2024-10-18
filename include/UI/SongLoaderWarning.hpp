#pragma once

#include "custom-types/shared/macros.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "HMUI/ViewController.hpp"
#include "Zenject/IInitializable.hpp"
#include "bsml/shared/BSML/Components/ModalView.hpp"

DECLARE_CLASS_CODEGEN_INTERFACES(SongCore::UI, SongLoaderWarning, System::Object, Zenject::IInitializable*) {
    DECLARE_CTOR(ctor, GlobalNamespace::MainMenuViewController* mainMenuViewController);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::MainMenuViewController*, _mainMenuViewController);
    DECLARE_INSTANCE_FIELD_PRIVATE(BSML::ModalView*, _warningModal);
    DECLARE_INSTANCE_FIELD_PRIVATE(HMUI::ViewController::DidActivateDelegate*, _mainMenuViewControllerDidActivateEvent);

    DECLARE_INSTANCE_METHOD(void, DontShowAgain);
    DECLARE_OVERRIDE_METHOD_MATCH(void, Initialize, &Zenject::IInitializable::Initialize);
    private:
        void MainMenuViewControllerDidShow(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);
};