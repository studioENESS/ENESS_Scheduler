#include "feature_wellesley.h"

#ifdef FEATURE_WELLESLEY

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>

#include "imgui.h"
#include "imguidatechooser.h"

#include "../app_state.h"
#include "../platform.h"
#include "schedule_list_common.h"

std::vector<ContentItem> content_items;

void storeContentItemDataToJson(const std::vector<ContentItem>& items, const std::string& filename) {
    nlohmann::json jsonData;
    for (const auto& item : items) {
        jsonData.push_back({
            {"name", item.name},
            {"scriptName", item.scriptName}
            });
    }

    std::ofstream outputFile(filename);
    outputFile << std::setw(4) << jsonData << std::endl;
    outputFile.close();
}

std::vector<ContentItem> loadContentItemDataFromJson(const std::string& filename) {
    std::vector<ContentItem> items;
    std::ifstream inputFile(filename);
    if (inputFile.is_open()) {
        nlohmann::json jsonData;
        inputFile >> jsonData;

        for (const auto& item : jsonData) {
            ContentItem newItem;
            newItem.name = item["name"];
            newItem.scriptName = item["scriptName"];
            items.push_back(newItem);
        }

        inputFile.close();
    }
    return items;
}

void Wellesley_Init()
{
    content_items = loadContentItemDataFromJson("C:\\Content\\scripts.json");
}

void Wellesley_LoadSchedule(nlohmann::json& jsonfile)
{
    for (auto item : g_vecSchedule)
    {
        delete item;
    }

    g_vecSchedule.clear();

    for (auto& item : jsonfile["schedule"])
    {
        if (item.is_null())
            continue;

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
        if (item.contains("ProgramID"))
            newItem->programID = item["ProgramID"];

        newItem->sScript = content_filename;
        newItem->sScriptExecutablePath = pixile_location;
        if (item.contains("Script"))
        {
            newItem->sScript = utf8_decode(item["Script"]);
        }
        if (item.contains("Executable"))
        {
            newItem->sScriptExecutablePath = utf8_decode(item["Executable"]);
        }

        g_vecSchedule.push_back(newItem);
    }

    std::sort(g_vecSchedule.begin(), g_vecSchedule.end(), compareByStartDate);
}

void Wellesley_SaveSchedule(nlohmann::json& jsonfile)
{
    int index = 0;
    for (const auto sched : g_vecSchedule)
    {
        jsonfile["schedule"][index]["index"] = index;
        jsonfile["schedule"][index]["StartDate"]["Year"] = sched->startDate.tm_year;
        jsonfile["schedule"][index]["StartDate"]["Month"] = sched->startDate.tm_mon;
        jsonfile["schedule"][index]["StartDate"]["Day"] = sched->startDate.tm_mday;
        jsonfile["schedule"][index]["StartDate"]["YearDay"] = sched->startDate.tm_yday;

        jsonfile["schedule"][index]["EndDate"]["Year"] = sched->endDate.tm_year;
        jsonfile["schedule"][index]["EndDate"]["Month"] = sched->endDate.tm_mon;
        jsonfile["schedule"][index]["EndDate"]["Day"] = sched->endDate.tm_mday;
        jsonfile["schedule"][index]["EndDate"]["YearDay"] = sched->endDate.tm_yday;
        jsonfile["schedule"][index]["ProgramID"] = sched->programID;
        jsonfile["schedule"][index]["Script"] = utf8_encode(sched->sScript);
        jsonfile["schedule"][index]["Executable"] = utf8_encode(sched->sScriptExecutablePath);
        index++;
    }
}

bool Wellesley_ResolveContentScript(uint32_t programID)
{
    std::filesystem::path sptPath = orig_content_filename;
    content_filename = sptPath.parent_path();
    content_filename.append(L"\\");
    content_filename.append(utf8_decode(content_items[programID].scriptName));
    return true;
}

bool Wellesley_ScriptChanged(int item, int lastItem)
{
    return content_items[item].scriptName != content_items[lastItem].scriptName;
}

void Wellesley_DrawCurrentContent(int item)
{
    ImGui::Text("Current Scheduled Content: ");
    ImGui::SameLine();
    ImGui::Text(content_items[item].name.c_str());
}

void Wellesley_DrawScheduleUI()
{
    DrawScheduleListUI(
        [](SItemSchedule* item) { return content_items[item->programID].name; },
        [](SItemSchedule* item) {
            if (ImGui::BeginCombo("Program", content_items[item->programID].name.c_str()))
            {
                for (int pr = 0; pr < content_items.size(); pr++)
                {
                    const bool is_selected = (item->programID == pr);

                    if (ImGui::Selectable(content_items[pr].name.c_str(), &is_selected))
                    {
                        item->programID = (uint8_t)pr;
                    }
                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();

            }
        });
}

int Wellesley_AdjustWindowHeight(int height)
{
    (void)height;
    return 400;
}

#endif // FEATURE_WELLESLEY
