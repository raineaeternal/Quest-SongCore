#include "hooking.hpp"

#include "BeatGames/Analytics/Events/TelemetryEventSender.hpp"
#include "Analytics/Model/TelemetryModel.hpp"

using namespace BeatGames::Analytics::Events;
using namespace Analytics::Model;

// Disable all telemetry events
MAKE_AUTO_HOOK_MATCH(TelemetryEventSender_SendTelemetryEvent, &TelemetryEventSender::SendTelemetryEvent, void, TelemetryEventSender* self, StringW eventName, StringW eventFamily, StringW stringValue, int32_t intValue, int32_t durationMS, ArrayW<StringW> tags)
{}

MAKE_AUTO_HOOK_MATCH(TelemetryModel_SendAppLoadingEvent, &TelemetryModel::SendAppLoadingEvent, void, TelemetryModel*, StringW eventName, int32_t msDuration)
{}

MAKE_AUTO_HOOK_MATCH(TelemetryModel_SendLevelLoadingEvent, &TelemetryModel::SendLevelLoadingEvent, void, TelemetryModel* self, StringW eventName, ::StringW levelKey, int32_t msDuration, int32_t intValue)
{}

MAKE_AUTO_HOOK_MATCH(TelemetryModel_SetEventDispatcher, &TelemetryModel::SetEventDispatcher, void, TelemetryModel* self, BeatGames::Analytics::AnalyticsEventsDispatcher* dispatcher)
{}