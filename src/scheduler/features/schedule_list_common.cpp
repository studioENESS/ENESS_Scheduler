#include "schedule_list_common.h"

#if defined(FEATURE_GOOGLE) || defined(FEATURE_WELLESLEY) || defined(FEATURE_SCRIPT_LIBRARY)

#include <algorithm>
#include <ctime>

#include "imgui.h"
#include "imguidatechooser.h"

#include "../platform.h"
#include "../time_logic.h"
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
    node_name.append(bHighlight ? "* " : "").append(itemPrefix(item));
    node_name.append(" From: ");
    static char startDateText[128]; strftime(startDateText, 128, "%b %d %Y", &item->startDate);
    node_name.append(startDateText);
    node_name.append(" To: ");
    static char endDateText[128]; strftime(endDateText, 128, "%b %d %Y", &item->endDate);
    node_name.append(endDateText);

    if (bHighlight)
    {
        node_name.append(" *");
    }

    node_name.append("###").append(std::to_string(item->index));
    bool node_open = ImGui::TreeNodeEx(node_name.c_str(), ImGuiSelectableFlags_SpanAllColumns);
    if (node_open)
    {
        if (!MultiScriptsFeatureEnabled)
        {
            ImGui::SetNextItemWidth(180);
            if (ImGui::DateChooser("Scheduled Start Date", item->startDate, "%b %d %Y"))
            {
                // do something.
                std::sort(g_vecSchedule.begin(), g_vecSchedule.end(), compareByStartDate);
            }

            ImGui::SetNextItemWidth(180);
            if (ImGui::DateChooser("Scheduled End Date", item->endDate, "%b %d %Y"))
            {
                // do something.
                std::sort(g_vecSchedule.begin(), g_vecSchedule.end(), compareByStartDate);
            }

            ImGui::SetNextItemWidth(180);
            drawItemBody(item);

            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(128, 0, 32, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(129, 64, 32, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(64, 0, 16, 255));
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
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(32, 0, 128, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(32, 0, 200, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(32, 0, 64, 255));

    bool bAddItem = ImGui::Button("+ Add Schedule Item");
    ImGui::PopStyleColor(3);

    bool bValidate = false;// ImGui::Button("Validate Schedule");

    AddScheduleItem(bAddItem, bValidate);

    ImGui::BeginChildFrame(2, ImGui::GetContentRegionAvail());
    for (auto& sched : g_vecSchedule)
    {
        createScheduleItem(sched, itemPrefix, drawItemBody);
    }
    // Lambda function to define the condition
    auto condition = [](SItemSchedule* x) { return x->deleteMe; };

    // Use remove_if to move elements satisfying the condition to the end of the vector
    g_vecSchedule.erase(std::remove_if(g_vecSchedule.begin(), g_vecSchedule.end(), condition), g_vecSchedule.end());

    ImGui::EndChildFrame();
}

#endif // FEATURE_GOOGLE || FEATURE_WELLESLEY || FEATURE_SCRIPT_LIBRARY
