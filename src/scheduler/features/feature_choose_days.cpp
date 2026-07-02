#include "feature_choose_days.h"

#ifdef FEATURE_CHOOSE_DAYS

#include "imgui.h"

#include "../app_state.h"
#include "../time_logic.h"
#include "../ui_main.h"

namespace {

constexpr const char* kDayNames[7] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

void SyncAllDayEndTimesFromGlobal()
{
    for (int i = 0; i < 7; i++)
    {
        end_hour_by_day[i] = end_hour;
        end_minute_by_day[i] = end_minute;
    }
}

} // namespace

void ChooseDays_LoadJson(nlohmann::json& timeJson)
{
    if (timeJson.contains("day") && timeJson["day"].is_array()) {
        for (int i = 0; i < 7; i++)
        {
            bDays[i] = (i < timeJson["day"].size() && timeJson["day"][i].is_boolean()) ? timeJson["day"][i].get<bool>() : false;
        }
    }

    g_bUsePerDayEndTimes = timeJson.contains("use_per_day_end") && timeJson["use_per_day_end"].is_boolean()
        ? timeJson["use_per_day_end"].get<bool>() : false;

    if (timeJson.contains("end_by_day") && timeJson["end_by_day"].is_array()) {
        for (int i = 0; i < 7; i++)
        {
            if (i >= timeJson["end_by_day"].size() || !timeJson["end_by_day"][i].is_object())
                continue;

            auto& dayEnd = timeJson["end_by_day"][i];
            if (dayEnd.contains("hour") && dayEnd["hour"].is_number_integer())
                end_hour_by_day[i] = dayEnd["hour"].get<int>();
            if (dayEnd.contains("minute") && dayEnd["minute"].is_number_integer())
                end_minute_by_day[i] = dayEnd["minute"].get<int>();
        }
    }
}

void ChooseDays_SaveJson(nlohmann::json& jsonfile)
{
    for (int i = 0; i < 7; i++)
    {
        jsonfile["time"]["day"][i] = bDays[i];
    }

    jsonfile["time"]["use_per_day_end"] = g_bUsePerDayEndTimes;
    for (int i = 0; i < 7; i++)
    {
        jsonfile["time"]["end_by_day"][i]["hour"] = end_hour_by_day[i];
        jsonfile["time"]["end_by_day"][i]["minute"] = end_minute_by_day[i];
    }
}

void ChooseDays_DrawUI()
{
    ImGui::Text("Active Days");
    ImGui::Checkbox("Monday", &bDays[1]); ImGui::SameLine();
    ImGui::Checkbox("Tuesday", &bDays[2]); ImGui::SameLine();
    ImGui::Checkbox("Wednesday", &bDays[3]); ImGui::SameLine();
    ImGui::Checkbox("Thursday", &bDays[4]);
    ImGui::Checkbox("Friday", &bDays[5]); ImGui::SameLine();
    ImGui::Checkbox("Saturday", &bDays[6]); ImGui::SameLine();
    ImGui::Checkbox("Sunday", &bDays[0]);

    if (ImGui::Checkbox("Per-day end times", &g_bUsePerDayEndTimes))
    {
        if (g_bUsePerDayEndTimes)
            SyncAllDayEndTimesFromGlobal();
    }

    if (g_bUsePerDayEndTimes)
    {
        ImGui::Text("Ending Time (per day)");
        for (int i = 0; i < 7; i++)
        {
            ImGui::Text("%s", kDayNames[i]); ImGui::SameLine();
            createTimeCombo(kDayNames[i], end_hour_by_day[i], end_minute_by_day[i]);
        }
        if (ImGui::Button("Copy global end time to all days"))
            SyncAllDayEndTimesFromGlobal();
    }
}

bool ChooseDays_IsValidDay(int start_hour, int start_minute, int end_hour, int end_minute)
{
    return IsValidDayOfWeek(start_hour, start_minute, end_hour, end_minute);
}

#endif // FEATURE_CHOOSE_DAYS
