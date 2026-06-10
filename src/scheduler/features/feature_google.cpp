#include "feature_google.h"

#ifdef FEATURE_GOOGLE

#include <string>

#include "imgui.h"
#include "imguidatechooser.h"

#include "../app_state.h"
#include "../time_logic.h"
#include "../ui_main.h"
#include "schedule_list_common.h"

void Google_LoadSchedule(nlohmann::json& jsonfile)
{
    for (auto item : g_vecSchedule)
    {
        delete item;
    }

    g_vecSchedule.clear();

    for (auto& item : jsonfile["schedule"])
    {
        auto newItem = new SItemSchedule;
        newItem->index = (int32_t)g_vecSchedule.size();
        ImGui::SetDateToday(&newItem->startDate);
        ImGui::SetDateToday(&newItem->endDate);
        newItem->index = item["index"];
        newItem->startDate.tm_year = item["StartDate"]["Year"];
        newItem->startDate.tm_mon = item["StartDate"]["Month"];
        newItem->startDate.tm_mday = item["StartDate"]["Day"];
        newItem->startDate.tm_yday = item["StartDate"]["YearDay"];
        newItem->endDate.tm_year = item["EndDate"]["Year"];
        newItem->endDate.tm_mon = item["EndDate"]["Month"];
        newItem->endDate.tm_mday = item["EndDate"]["Day"];
        newItem->endDate.tm_yday = item["EndDate"]["YearDay"];
        newItem->start_hour = item["time"]["start"]["hour"];
        newItem->start_minute = item["time"]["start"]["minute"];
        newItem->end_hour = item["time"]["end"]["hour"];
        newItem->end_minute = item["time"]["end"]["minute"];


        g_vecSchedule.push_back(newItem);
    }
}

void Google_SaveSchedule(nlohmann::json& jsonfile)
{
    for (const auto sched : g_vecSchedule)
    {
        jsonfile["schedule"][sched->index]["index"] = sched->index;
        jsonfile["schedule"][sched->index]["StartDate"]["Year"] = sched->startDate.tm_year;
        jsonfile["schedule"][sched->index]["StartDate"]["Month"] = sched->startDate.tm_mon;
        jsonfile["schedule"][sched->index]["StartDate"]["Day"] = sched->startDate.tm_mday;
        jsonfile["schedule"][sched->index]["StartDate"]["YearDay"] = sched->startDate.tm_yday;

        jsonfile["schedule"][sched->index]["EndDate"]["Year"] = sched->endDate.tm_year;
        jsonfile["schedule"][sched->index]["EndDate"]["Month"] = sched->endDate.tm_mon;
        jsonfile["schedule"][sched->index]["EndDate"]["Day"] = sched->endDate.tm_mday;
        jsonfile["schedule"][sched->index]["EndDate"]["YearDay"] = sched->endDate.tm_yday;
        jsonfile["schedule"][sched->index]["time"]["start"]["hour"] = sched->start_hour;
        jsonfile["schedule"][sched->index]["time"]["start"]["minute"] = sched->start_minute;
        jsonfile["schedule"][sched->index]["time"]["end"]["hour"] = sched->end_hour;
        jsonfile["schedule"][sched->index]["time"]["end"]["minute"] = sched->end_minute;
    }
}

bool Google_CheckScheduleItemTime(tm* localTime, int scheduleItem, bool& inTime, bool& validDay)
{
    if (scheduleItem == -1)
        return false;

    inTime = isTimeBetween(localTime, g_vecSchedule[scheduleItem]->start_hour,
        g_vecSchedule[scheduleItem]->start_minute,
        g_vecSchedule[scheduleItem]->end_hour,
        g_vecSchedule[scheduleItem]->end_minute);
    validDay = true;
    return true;
}

void Google_DrawScheduleUI()
{
    DrawScheduleListUI(
        [](SItemSchedule*) { return std::string(); },
        [](SItemSchedule* item) {
            ImGui::Text("Staring Time"); ImGui::SameLine();
            createTimeCombo("Scheduled Start Time (Per Day)", item->start_hour, item->start_minute);

            ImGui::Text("Ending Time"); ImGui::SameLine();
            createTimeCombo("Scheduled End Time (Per Day)", item->end_hour, item->end_minute);
            if (scheduleCrossesMidnight(item->start_hour, item->start_minute, item->end_hour, item->end_minute))
                ImGui::TextDisabled("(end time is the next day)");
        });
}

int Google_AdjustWindowHeight(int height)
{
    (void)height;
    return 400;
}

#endif // FEATURE_GOOGLE
