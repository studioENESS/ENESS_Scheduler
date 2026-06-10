#include "feature_choose_days.h"

#ifdef FEATURE_CHOOSE_DAYS

#include "imgui.h"

#include "../app_state.h"
#include "../time_logic.h"

void ChooseDays_LoadJson(nlohmann::json& timeJson)
{
    if (timeJson.contains("day") && timeJson["day"].is_array()) {
        for (int i = 0; i < 7; i++)
        {
            bDays[i] = (i < timeJson["day"].size() && timeJson["day"][i].is_boolean()) ? timeJson["day"][i].get<bool>() : false;
        }
    }
}

void ChooseDays_SaveJson(nlohmann::json& jsonfile)
{
    for (int i = 0; i < 7; i++)
    {
        jsonfile["time"]["day"][i] = bDays[i];
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
}

bool ChooseDays_IsValidDay(int start_hour, int start_minute, int end_hour, int end_minute)
{
    return IsValidDayOfWeek(start_hour, start_minute, end_hour, end_minute);
}

#endif // FEATURE_CHOOSE_DAYS
