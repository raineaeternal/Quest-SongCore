#include "hooking.hpp"
#include "logging.hpp"

#include "GlobalNamespace/SaberManager.hpp"

bool OneSaberOverrideActive = false;
bool OneSaberOverrideForceOneSaber = false;

MAKE_AUTO_HOOK_MATCH(SaberManager_Start, &GlobalNamespace::SaberManager::Start, void, GlobalNamespace::SaberManager* self) {
    if (OneSaberOverrideActive) {
        self->_initData = GlobalNamespace::SaberManager::InitData::New_ctor(OneSaberOverrideForceOneSaber, self->_initData->oneSaberType);
    }

    SaberManager_Start(self);
}
