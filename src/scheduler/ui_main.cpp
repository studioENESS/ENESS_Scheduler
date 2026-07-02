#include "ui_main.h"

#include <ctime>
#include <memory>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"

#include "app_state.h"
#include "platform.h"
#include "schedule_io.h"
#include "time_logic.h"
#include "features/feature_audio.h"
#include "features/feature_choose_days.h"
#include "features/feature_csl.h"
#include "features/feature_google.h"
#include "features/feature_multiscripts.h"
#include "features/feature_wellesley.h"
#include "features/feature_script_library.h"

static int GetCurrentScheduledItem()
{

    time_t currentTime = time(0);
    tm* localTime = new tm();
    localtime_s(localTime, &currentTime);

    int index = 0;
    for (auto& item : g_vecSchedule)
    {
        if (isDateBetween(localTime, &item->startDate, &item->endDate))
        {
            if (localTime)
                delete localTime;
            if (GoogleFeatureEnabled || ScriptLibraryFeatureEnabled)
                return index;
            return item->programID;
        }
        index++;
    }

    if (localTime)
        delete localTime;
    if (GoogleFeatureEnabled || ScriptLibraryFeatureEnabled)
        return -1;
    return 12;
}


void createTimeCombo(std::string sComboName, int& current_hour_idx, int& current_min_idx)
{
    ImGui::PushID(sComboName.c_str());
    // Hour
    {
        auto old_str = std::to_string(current_hour_idx);
        auto new_str = std::string(2 - mymin(2, old_str.length()), '0') + old_str;
        const char* combo_preview_value = new_str.c_str();  // Pass in the preview value visible before opening the combo (it could be anything)
        ImGui::SetNextItemWidth(80);
        if (ImGui::BeginCombo("###HourTime", combo_preview_value))
        {
            for (int n = 0; n < 24; n++)
            {

                const bool is_selected = (current_hour_idx == n);
                auto old_str2 = std::to_string(n);
                auto new_str2 = std::string(2 - mymin(2, old_str2.length()), '0') + old_str2;

                if (ImGui::Selectable(new_str2.c_str(), &is_selected))
                {
                    current_hour_idx = n;
                }
                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
    ImGui::SameLine();
    // Minute
    {
        auto old_str = std::to_string(current_min_idx);
        auto new_str = std::string(2 - mymin(2, old_str.length()), '0') + old_str;
        const char* combo_preview_value = new_str.c_str();  // Pass in the preview value visible before opening the combo (it could be anything)
        ImGui::SetNextItemWidth(80);
        if (ImGui::BeginCombo("###MinuteTime", combo_preview_value))
        {

            for (int n = 0; n < 60; n++)
            {

                const bool is_selected = (current_min_idx == n);
                auto old_str3 = std::to_string(n);
                auto new_str3 = std::string(2 - mymin(2, old_str3.length()), '0') + old_str3;

                if (ImGui::Selectable(new_str3.c_str(), &is_selected))
                {
                    current_min_idx = n;
                }
                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
    ImGui::PopID();
}

void pushStyleColours18(float h, bool active) {
    while (h > 1) h = h - 1;
    float offset = 0.45f;
    float o = h + offset;
    while (o > 1) o = o - 1;
    //float v = 0.7;
    float v = 0;
    float grey = 0.2f;

    //float s = 0.5;
    float s = 0.0f;

    ImGui::PushStyleColor(ImGuiCol_CheckMark, (ImVec4)ImColor::HSV(h, s, 1.f));

    ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(h, s, 0.35f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, (ImVec4)ImColor::HSV(h, s, 0.35f));

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(h, s, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(h, s, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(h, 0.5f, 0.7f));

    ImGui::PushStyleColor(ImGuiCol_Header, (ImVec4)ImColor::HSV(h, active ? 0.25f : s, active ? 0.35f : v));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, (ImVec4)ImColor::HSV(h, s, v));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, (ImVec4)ImColor::HSV(h, 0.5f, 0.7f));

    ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(h, s, grey));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(h, s, grey));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(h, 0.5f, 0.7f));

    ImGui::PushStyleColor(ImGuiCol_Tab, (ImVec4)ImColor::HSV(h, s, v));
    ImGui::PushStyleColor(ImGuiCol_TabActive, (ImVec4)ImColor::HSV(h, 0.5f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_TabHovered, (ImVec4)ImColor::HSV(h, s, grey));
    ImGui::PushStyleColor(ImGuiCol_TabUnfocused, (ImVec4)ImColor::HSV(h, s, v));
    ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, (ImVec4)ImColor::HSV(h, s, v));
    ImGui::PushStyleColor(ImGuiCol_Separator, (ImVec4)ImColor::HSV(h, 0.5f, 0.7f));
}

void StyleColorsPhotoshop()
{
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
    colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
    colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
    colors[ImGuiCol_TabActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    // colors[ImGuiCol_DockingPreview] = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
    // colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

    style->ChildRounding = 4.0f;
    style->FrameBorderSize = 1.0f;
    style->FrameRounding = 2.0f;
    style->GrabMinSize = 7.0f;
    style->PopupRounding = 2.0f;
    style->ScrollbarRounding = 12.0f;
    style->ScrollbarSize = 13.0f;
    style->TabBorderSize = 1.0f;
    style->TabRounding = 0.0f;
    style->WindowRounding = 4.0f;
}

void DrawMainGUI()
{
    static int lastItem = -1;
    static std::string lastCatalogId;
    static int item = 0;
    item = GetCurrentScheduledItem();
    if (ScriptLibraryFeatureEnabled && item >= 0 && item < (int)g_vecSchedule.size())
    {
        const std::string& catalogId = g_vecSchedule[item]->catalog_id;
        if (catalogId != lastCatalogId)
        {
            if (!lastCatalogId.empty())
                killPlayer();
            lastCatalogId = catalogId;
        }
        ScriptLibrary_ApplyEntry(catalogId);
    }

    if (item != lastItem)
    {
        SetCurrentProgram(item);
    }
    if (lastItem != item)
    {
        if (lastItem != -1 || item != -1)
        {
            killPlayer();
        }
        else if (ScriptLibraryFeatureEnabled && ScriptLibrary_ScriptChanged(item, lastItem))
        {
            killPlayer();
        }
        else if (Wellesley_ScriptChanged(item, lastItem))
        {
            killPlayer();
        }
    }
    lastItem = item;
#ifdef IMGUI_HAS_VIEWPORT
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetWorkPos());
    ImGui::SetNextWindowSize(viewport->GetWorkSize());
    ImGui::SetNextWindowViewport(viewport->ID);
#else
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
#endif
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    ImGui::Begin("ENESS Scheduler", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

    auto running = isProcessRunning(L"player.exe", item);

    ImGui::Text("Current Player Status:");
    ImGui::SameLine();
    switch (running)
    {
    case PIXILE_STATUS_RUNNING:
        ImGui::TextColored(ImColor(IM_COL32(0, 255, 0, 255)), "Running");
        break;
    case PIXILE_STATUS_NOTSCHEDULED:
        ImGui::TextColored(ImColor(IM_COL32(255, 255, 0, 255)), "Out of Scheduled Time");
        killPlayer();
        break;
    case PIXILE_STATUS_OFF:
    default:
        ImGui::TextColored(ImColor(IM_COL32(255, 0, 0, 255)), "Not Running");
        startPlayer(item);
        break;
    }



    std::string sPlayerStartStr;
    if (running == PIXILE_STATUS_RUNNING)
    {
        ImGui::SameLine();
        sPlayerStartStr.append("Restart Lumes");

        sPlayerStartStr.append("##StartPlayer");
        if (ImGui::Button(sPlayerStartStr.c_str()))
        {
            killPlayer();
        }

    }
    if (g_bCanUseAlternatePlayer)
    {
        if (ImGui::Checkbox("Disable People Tracking", &g_bUseAlternatePlayer))
        {
            killPlayer();

        }
    }

    if (ScriptLibraryFeatureEnabled)
        ScriptLibrary_DrawCurrentContent(item);
    else if (MultiScriptsFeatureEnabled)
    {
        ImGui::Text("Current Scheduled Content: ");
        ImGui::SameLine();
    }
    else
    {
        Wellesley_DrawCurrentContent(item);
    }

    ImGui::Text("Staring Time"); ImGui::SameLine();
    createTimeCombo("Scheduled Start Time (Per Day)", start_hour, start_minute);

    if (!g_bUsePerDayEndTimes)
    {
        ImGui::Text("Ending Time"); ImGui::SameLine();
        createTimeCombo("Scheduled End Time (Per Day)", end_hour, end_minute);
        if (scheduleCrossesMidnight(start_hour, start_minute, end_hour, end_minute))
            ImGui::TextDisabled("(end time is the next day)");
    }

    ChooseDays_DrawUI();

    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 128, 32, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(64, 128, 32, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 64, 16, 255));

    bool bLoadSchedule = ImGui::Button("Load Schedule");
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(128, 0, 32, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(129, 64, 32, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(64, 0, 16, 255));
    bool bSaveSchedule = ImGui::Button("Save Schedule");
    ImGui::PopStyleColor(3);

    if (bLoadSchedule)
    {
#ifdef USE_HARD_PATHS
        loadSchedule(SCHEDULER_DEFAULT_SCHEDULE_PATH);
#else
        std::vector<std::string> filters = { "Lumes Schedule", "*.lsc" };
        open_file = std::make_shared<pfd::open_file>("Choose file", "C:\\", filters);
#endif

    }

    if (bSaveSchedule)
    {
#ifdef USE_HARD_PATHS
        saveSchedule(SCHEDULER_DEFAULT_SCHEDULE_PATH);
#else
        std::vector<std::string> filters = { "Lumes Schedule", "*.lsc" };
        save_file = std::make_shared<pfd::save_file>("Choose file", "C:\\", filters);
#endif
    }

    ScriptLibrary_DrawUI();

    Google_DrawScheduleUI();
    Wellesley_DrawScheduleUI();
    ScriptLibrary_DrawScheduleUI();

    MultiScripts_DrawUI();

    Audio_DrawUI();

    CSL_DrawUI();

    ImGui::End();
    ImGui::PopStyleVar(1);
}
