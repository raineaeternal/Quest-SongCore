#include "hooking.hpp"
#include "logging.hpp"

#include "GlobalNamespace/BpmTimeProcessor.hpp"
#include "BeatmapSaveDataVersion2_6_0AndEarlier/BeatmapSaveDataVersion2_6_0AndEarlier.hpp"

#include "System/Collections/Generic/IReadOnlyList_1.hpp"
#include "System/Collections/Generic/IReadOnlyCollection_1.hpp"

MAKE_AUTO_HOOK_MATCH(
    BpmTimeProcessorConstructor,
    static_cast<void (GlobalNamespace::BpmTimeProcessor::*)(float, System::Collections::Generic::IReadOnlyList_1<BeatmapSaveDataVersion2_6_0AndEarlier::EventData *> *)>(&GlobalNamespace::BpmTimeProcessor::_ctor),
    void, GlobalNamespace::BpmTimeProcessor *self, float startBpm, ::System::Collections::Generic::IReadOnlyList_1<::BeatmapSaveDataVersion2_6_0AndEarlier::EventData *> *events)
{
    BpmTimeProcessorConstructor(self, startBpm, events);

    std::vector<BeatmapSaveDataVersion2_6_0AndEarlier::EventData *> bpmEvents;
    for (int i = 0; i < ((System::Collections::Generic::IReadOnlyCollection_1<BeatmapSaveDataVersion2_6_0AndEarlier::EventData *> *)events)->Count; i++)
    {
        auto event = events->Item[i];
        if (event->type == BeatmapSaveDataCommon::BeatmapEventType::BpmChange)
        {
            bpmEvents.push_back(event);
        }
    }

    bool startsAtZero = !bpmEvents.empty() && bpmEvents[0]->time == 0.0f;
    if (startsAtZero)
    {
        startBpm = bpmEvents[0]->floatValue;
    }

    self->_bpmChangeDataList->set_Item(0, GlobalNamespace::__BpmTimeProcessor__BpmChangeData(0.0f, 0.0f, startBpm));
    int bpmIndex = 1;
    for (size_t index = startsAtZero ? 1 : 0; index < bpmEvents.size(); ++index)
    {
        auto prevBpmChangeData = self->_bpmChangeDataList->Item[bpmIndex - 1];
        float time = bpmEvents[index]->time;
        float floatValue = bpmEvents[index]->floatValue;

        self->_bpmChangeDataList->Item[bpmIndex] = GlobalNamespace::__BpmTimeProcessor__BpmChangeData(
            GlobalNamespace::BpmTimeProcessor::CalculateTime(prevBpmChangeData, time),
            time,
            floatValue);
        bpmIndex++;
    }
}