#include "schedule_list_common.h"

#if defined(FEATURE_GOOGLE) || defined(FEATURE_WELLESLEY) || defined(FEATURE_SCRIPT_LIBRARY)

#include <algorithm>
#include <ctime>

#include "imgui.h"
#include "imguidatechooser.h"

#include "../platform.h"
#include "../time_logic.h"
#include "../ui_main.h"
#include "feature_multiscripts.h"
#include "feature_script_library.h"

void AddScheduleItem(bool bAddItem, bool bValidate)
{
    if (bAddItem)
    {
        auto newItem = new SItemSchedule;
        newItem->index = (uint32_t)g_vecSchedule.size();
        ImGui::SetDateToday(&newItem->startDate);
        ImGui::SetDateToday(&newItem->endDate);
        newItem->programID = 0;
#ifdef FEATURE_SCRIPT_LIBRARY
        if (!g_scriptCatalog.empty())
            newItem->catalog_id = g_scriptCatalog[0].id;
#endif
        g_vecSchedule.push_back(newItem);
    }

    if (bValidate)
    {
    }
}

static void createScheduleItem(SItemSchedule* item,
                               const std::function<std::string(SItemSchedule*)>& itemPrefix,
                               const std::function<void(SItemSchedule*)>& drawItemBody)
{
    bool bHighlight = false;
    time_t currentTime = time(0);
    tm* localTime = new tm();
    localtime_s(localTime, &currentTime);

    bHighlight = isDateBetween(localTime, &item->startDate, &item->endDate);
    delete localTime;
    ImGui::PushID(item->index);
    std::string node_name;
    node_name.append(bHighlight ? "● " : "").append(itemPrefix(item));
    node_name.append("   ");
    static char startDateText[128]; strftime(startDateText, 128, "%b %d %Y", &item->startDate);
    node_name.append(startDateText);
    node_name.append(" → ");
    static char endDateText[128]; strftime(endDateText, 128, "%b %d %Y", &item->endDate);
    node_name.append(endDateText);

    if (bHighlight)
    {
        node_name.append("  ·  Active");
    }

    node_name.append("###").append(std::to_string(item->index));
    bool node_open = ImGui::TreeNodeEx(node_name.c_str(), ImGuiSelectableFlags_SpanAllColumns);
    if (node_open)
    {
        if (!MultiScriptsFeatureEnabled)
        {
            ImGui::SetNextItemWidth(UIPx(180.0f));
            if (ImGui::DateChooser("Scheduled Start Date", item->startDate, "%b %d %Y"))
            {
                std::sort(g_vecSchedule.begin(), g_vecSchedule.end(), compareByStartDate);
            }

            ImGui::SetNextItemWidth(UIPx(180.0f));
            if (ImGui::DateChooser("Scheduled End Date", item->endDate, "%b %d %Y"))
            {
                std::sort(g_vecSchedule.begin(), g_vecSchedule.end(), compareByStartDate);
            }

            ImGui::SetNextItemWidth(UIPx(180.0f));
            drawItemBody(item);

            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.45f, 0.18f, 0.20f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.58f, 0.22f, 0.25f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.68f, 0.26f, 0.28f, 1.00f));
            if (ImGui::Button("Delete Item"))
            {
                item->deleteMe = true;
            }
            ImGui::PopStyleColor(3);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void DrawScheduleListUI(const std::function<std::string(SItemSchedule*)>& itemPrefix,
                        const std::function<void(SItemSchedule*)>& drawItemBody)
{
    ImGui::SameLine(0.0f, UIPx(8.0f));
    bool bAddItem = ImGui::Button("+ Add Schedule Item");
    bool bValidate = false;

    AddScheduleItem(bAddItem, bValidate);

    ImGui::Spacing();
    ImGui::BeginChild("schedule-items", ImGui::GetContentRegionAvail(), true);
    for (auto& sched : g_vecSchedule)
    {
        createScheduleItem(sched, itemPrefix, drawItemBody);
    }
    auto condition = [](SItemSchedule* x) { return x->deleteMe; };
    g_vecSchedule.erase(std::remove_if(g_vecSchedule.begin(), g_vecSchedule.end(), condition), g_vecSchedule.end());

    ImGui::EndChild();
}

#endif // FEATURE_GOOGLE || FEATURE_WELLESLEY || FEATURE_SCRIPT_LIBRARY
