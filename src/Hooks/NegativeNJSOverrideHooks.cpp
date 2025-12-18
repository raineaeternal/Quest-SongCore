#include "hooking.hpp"
#include "logging.hpp"
#include "config.hpp"

#include "GlobalNamespace/BeatmapDifficultyMethods.hpp"
#include "GlobalNamespace/VariableMovementDataProvider.hpp"

#include "UnityEngine/Mathf.hpp"

/// @brief Allows negative note jump speeds, which are otherwise reset to the default NJS value.
/// When a negative NJS is used, notes will come from behind the player and spin toward them.
///
/// @example https://beatsaver.com/maps/6cd
MAKE_AUTO_HOOK_MATCH(BeatmapDifficultyMethods_NoteJumpMovementSpeed, &GlobalNamespace::BeatmapDifficultyMethods::NoteJumpMovementSpeed, float, GlobalNamespace::BeatmapDifficulty difficulty, float_t noteJumpMovementSpeed, bool fastNotes)
{
    float result = BeatmapDifficultyMethods_NoteJumpMovementSpeed(difficulty, noteJumpMovementSpeed, fastNotes);

    if(noteJumpMovementSpeed <= -GlobalNamespace::VariableMovementDataProvider::kMinNoteJumpMovementSpeed)
    {
        result = noteJumpMovementSpeed;
    }

    return result;
}

/// Transpiler to edit target jump speed https://github.com/Kylemc1413/SongCore/blob/master/source/SongCore/Patches/AllowNegativeNoteJumpSpeedPatch.cs#L32-L53
///
/// @brief By default, the provider uses the highest note jump speed value, capping it at VariableMovementDataProvider::kMinNoteJumpMovementSpeed.
/// This patch allows it to also use the lowest NJS value when the initial one is negative, capping it at -VariableMovementDataProviderkMinNoteJumpMovementSpeed.
MAKE_AUTO_HOOK_MATCH(VariableMovementDataProvider_ManualUpdate, &GlobalNamespace::VariableMovementDataProvider::ManualUpdate, void, GlobalNamespace::VariableMovementDataProvider* self, float songTime)
{
    /* Transpiler Start */
    auto noteJumpSpeed = self->_initNoteJumpMovementSpeed + self->_relativeNoteJumpSpeedInterpolation.GetValue(songTime);
    auto positiveNoteJumpMovementSpeed = std::max(noteJumpSpeed, GlobalNamespace::VariableMovementDataProvider::kMinNoteJumpMovementSpeed);
    auto negativeNoteJumpMovementSpeed = std::min(noteJumpSpeed, -GlobalNamespace::VariableMovementDataProvider::kMinNoteJumpMovementSpeed);
    self->_targetNoteJumpMovementSpeed = self->_initNoteJumpMovementSpeed > 0 ? positiveNoteJumpMovementSpeed : negativeNoteJumpMovementSpeed;
    /* Transpiler End */
    self->wasUpdatedThisFrame = false;
    if(!UnityEngine::Mathf::Approximately(self->_prevNoteJumpMovementSpeed, self->_targetNoteJumpMovementSpeed))
    {
        self->wasUpdatedThisFrame = true;
        self->_prevNoteJumpMovementSpeed = self->_noteJumpMovementSpeed;
        self->_noteJumpMovementSpeed = self->_targetNoteJumpMovementSpeed;
        if(self->_noteJumpValueType == GlobalNamespace::BeatmapObjectSpawnMovementData::NoteJumpValueType::BeatOffset)
        {
            float num = std::min(self->_noteJumpMovementSpeed / self->_initNoteJumpMovementSpeed, 1.0f);
            self->_halfJumpDuration = self->_initOneBeatDuration * self->_halfJumpDurationInBeats / num;
            self->_jumpDuration = self->_halfJumpDuration * 2.0f;
        }
        self->_waitingDuration = self->_spawnAheadTime - 0.5f - self->_halfJumpDuration;
        self->_jumpDistance = self->_noteJumpMovementSpeed * self->_jumpDuration;
        self->_halfJumpDistance = self->_jumpDistance * 0.5f;
        self->_moveStartPosition = UnityEngine::Vector3::op_Addition(self->_centerPosition, UnityEngine::Vector3::op_Multiply(self->_forwardVector, 100.0f + self->_halfJumpDistance));
        self->_moveEndPosition = UnityEngine::Vector3::op_Addition(self->_centerPosition, UnityEngine::Vector3::op_Multiply(self->_forwardVector, self->_halfJumpDistance));
        self->_jumpEndPosition = UnityEngine::Vector3::op_Subtraction(self->_centerPosition, UnityEngine::Vector3::op_Multiply(self->_forwardVector, self->_halfJumpDistance));
    }
}
