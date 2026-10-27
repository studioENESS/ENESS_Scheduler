#include "ui_main.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"
#include "imguidatechooser.h"

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

namespace {

float g_uiScale = 1.0f;

bool FileExists(const char* path)
{
    if (!path || !*path)
        return false;
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

std::string ShortScriptDisplay(const std::wstring& wpath)
{
    std::string s = utf8_encode(wpath);
    if (s.empty())
        return "(none)";
    const auto pos = s.find_last_of("/\\");
    if (pos != std::string::npos && pos + 1 < s.size())
        return s.substr(pos + 1);
    return s;
}

} // namespace

float GetUIScale()
{
    return g_uiScale;
}

float UIPx(float value)
{
    return value * g_uiScale;
}

void DrawSectionHeader(const char* title, const char* hint)
{
    ImGui::Spacing();
    ImGui::TextUnformatted(title);
    ImGui::Separator();
    if (hint && *hint)
        ImGui::TextDisabled("%s", hint);
}

void SetupFonts(float scale)
{
    if (scale <= 0.0f)
        scale = 1.0f;
    g_uiScale = scale;

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

#ifdef _WIN32
    const std::string candidates[] = {
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
    };
#else
    const std::string candidates[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
    };
#endif

    ImFont* loaded = nullptr;
    for (const auto& path : candidates)
    {
        if (FileExists(path.c_str()))
        {
            loaded = io.Fonts->AddFontFromFileTTF(path.c_str(), 15.0f * scale);
            if (loaded)
                break;
        }
    }

    if (!loaded)
    {
        io.Fonts->AddFontDefault();
        io.FontGlobalScale = scale;
    }
}

void ScaleStyleForDPI(float scale)
{
    if (scale <= 1.01f)
        return;
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(scale);
}

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
    char hourPreview[8];
    std::snprintf(hourPreview, sizeof(hourPreview), "%02d", current_hour_idx);
    ImGui::SetNextItemWidth(UIPx(68.0f));
    if (ImGui::BeginCombo("##HourTime", hourPreview))
    {
        for (int n = 0; n < 24; n++)
        {
            const bool is_selected = (current_hour_idx == n);
            char itemLabel[8];
            std::snprintf(itemLabel, sizeof(itemLabel), "%02d", n);
            if (ImGui::Selectable(itemLabel, is_selected))
                current_hour_idx = n;
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine(0.0f, UIPx(4.0f));
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled(":");
    ImGui::SameLine(0.0f, UIPx(4.0f));

    char minPreview[8];
    std::snprintf(minPreview, sizeof(minPreview), "%02d", current_min_idx);
    ImGui::SetNextItemWidth(UIPx(68.0f));
    if (ImGui::BeginCombo("##MinuteTime", minPreview))
    {
        for (int n = 0; n < 60; n++)
        {
            const bool is_selected = (current_min_idx == n);
            char itemLabel[8];
            std::snprintf(itemLabel, sizeof(itemLabel), "%02d", n);
            if (ImGui::Selectable(itemLabel, is_selected))
                current_min_idx = n;
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();
}

void pushStyleColours18(float h, bool active)
{
    while (h > 1) h = h - 1;
    float v = 0;
    float grey = 0.2f;
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
    // Modern dark teal theme matching sync_to_pis.py / imgui_app.py
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 8.0f;
    style.PopupRounding     = 8.0f;
    style.FrameRounding     = 6.0f;
    style.GrabRounding      = 6.0f;
    style.TabRounding       = 6.0f;
    style.ScrollbarRounding = 8.0f;

    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.PopupBorderSize   = 1.0f;
    style.TabBorderSize     = 0.0f;

    style.WindowPadding     = ImVec2(16.0f, 14.0f);
    style.FramePadding      = ImVec2(12.0f, 7.0f);
    style.ItemSpacing       = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
    style.CellPadding       = ImVec2(8.0f, 6.0f);
    style.ScrollbarSize     = 12.0f;
    style.GrabMinSize       = 12.0f;

    const ImVec4 ink        = ImVec4(0.93f, 0.94f, 0.95f, 1.00f);
    const ImVec4 muted      = ImVec4(0.62f, 0.66f, 0.70f, 1.00f);
    const ImVec4 bg         = ImVec4(0.11f, 0.12f, 0.14f, 1.00f);
    const ImVec4 raised     = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
    const ImVec4 field      = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    const ImVec4 field_hot  = ImVec4(0.25f, 0.28f, 0.32f, 1.00f);
    const ImVec4 line       = ImVec4(0.28f, 0.30f, 0.34f, 1.00f);
    const ImVec4 accent     = ImVec4(0.29f, 0.62f, 0.60f, 1.00f);
    const ImVec4 accent_hot = ImVec4(0.36f, 0.72f, 0.69f, 1.00f);
    const ImVec4 accent_dim = ImVec4(0.18f, 0.36f, 0.36f, 1.00f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                  = ink;
    colors[ImGuiCol_TextDisabled]          = muted;
    colors[ImGuiCol_WindowBg]              = bg;
    colors[ImGuiCol_ChildBg]               = raised;
    colors[ImGuiCol_PopupBg]               = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_Border]                = line;
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = field;
    colors[ImGuiCol_FrameBgHovered]        = field_hot;
    colors[ImGuiCol_FrameBgActive]         = field_hot;
    colors[ImGuiCol_TitleBg]               = bg;
    colors[ImGuiCol_TitleBgActive]         = bg;
    colors[ImGuiCol_TitleBgCollapsed]      = bg;
    colors[ImGuiCol_MenuBarBg]             = raised;
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.32f, 0.35f, 0.39f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.40f, 0.44f, 0.48f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = accent;
    colors[ImGuiCol_CheckMark]             = accent_hot;
    colors[ImGuiCol_SliderGrab]            = accent;
    colors[ImGuiCol_SliderGrabActive]      = accent_hot;
    colors[ImGuiCol_Button]                = ImVec4(0.24f, 0.26f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.30f, 0.34f, 0.38f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = accent_dim;
    colors[ImGuiCol_Header]                = accent_dim;
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.24f, 0.46f, 0.45f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = accent;
    colors[ImGuiCol_Separator]             = line;
    colors[ImGuiCol_SeparatorHovered]      = accent;
    colors[ImGuiCol_SeparatorActive]       = accent_hot;
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.29f, 0.62f, 0.60f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]     = accent;
    colors[ImGuiCol_ResizeGripActive]      = accent_hot;
    colors[ImGuiCol_Tab]                   = raised;
    colors[ImGuiCol_TabHovered]            = accent_dim;
    colors[ImGuiCol_TabActive]             = ImVec4(0.20f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.14f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.18f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.18f, 0.20f, 0.23f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = line;
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.22f, 0.24f, 0.27f, 1.00f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
    colors[ImGuiCol_PlotLines]             = accent;
    colors[ImGuiCol_PlotLinesHovered]      = accent_hot;
    colors[ImGuiCol_PlotHistogram]         = accent;
    colors[ImGuiCol_PlotHistogramHovered]  = accent_hot;
    colors[ImGuiCol_TextSelectedBg]        = accent_dim;
    colors[ImGuiCol_DragDropTarget]        = accent_hot;
    colors[ImGuiCol_NavHighlight]          = accent;
    colors[ImGuiCol_NavWindowingHighlight] = accent_hot;
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.00f, 0.00f, 0.00f, 0.58f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.00f, 0.00f, 0.00f, 0.58f);
}

static void DrawFormLabel(const char* label, float labelColumnWidth)
{
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(labelColumnWidth);
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
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    const ImGuiWindowFlags mainFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("ENESS Scheduler", nullptr, mainFlags);

    const ImVec4 colOk   = ImVec4(0.45f, 0.85f, 0.55f, 1.00f);
    const ImVec4 colWarn = ImVec4(0.90f, 0.70f, 0.30f, 1.00f);
    const ImVec4 colErr  = ImVec4(0.95f, 0.40f, 0.40f, 1.00f);

    const float labelCol = UIPx(148.0f);

    // -----------------------------------------------------------------------
    // Status Section
    // -----------------------------------------------------------------------
    DrawSectionHeader("Status");

    auto running = isProcessRunning(L"player.exe", item);

    DrawFormLabel("Player", labelCol);
    switch (running)
    {
    case PIXILE_STATUS_RUNNING:
        ImGui::TextColored(colOk, "Running");
        break;
    case PIXILE_STATUS_NOTSCHEDULED:
        ImGui::TextColored(colWarn, "Out of Scheduled Time");
        killPlayer();
        break;
    case PIXILE_STATUS_OFF:
    default:
        ImGui::TextColored(colErr, "Not Running");
        startPlayer(item);
        break;
    }

    if (running == PIXILE_STATUS_RUNNING)
    {
        ImGui::SameLine(0.0f, UIPx(12.0f));
        if (ImGui::Button("Restart Lumes##StartPlayer"))
        {
            killPlayer();
        }
    }

    if (g_bCanUseAlternatePlayer)
    {
        DrawFormLabel("Tracking", labelCol);
        if (ImGui::Checkbox("Disable People Tracking", &g_bUseAlternatePlayer))
        {
            killPlayer();
        }
    }

    if (HasTwoScripts())
    {
        bool lightsOn = false;
        bool soundsOn = false;
        UpdateGpioSchedules(lightsOn, soundsOn);

        char lightsLabel[32];
        std::snprintf(lightsLabel, sizeof(lightsLabel), "Lights (GPIO %d)", GPIO_PIN_LIGHTS);
        DrawFormLabel(lightsLabel, labelCol);
        if (lightsOn)
            ImGui::TextColored(colOk, "ON");
        else
            ImGui::TextColored(colWarn, "OFF  ·  Out of Scheduled Time");

        char soundsLabel[32];
        std::snprintf(soundsLabel, sizeof(soundsLabel), "Sounds (GPIO %d)", GPIO_PIN_SOUNDS);
        DrawFormLabel(soundsLabel, labelCol);
        if (soundsOn)
        {
            ImGui::TextColored(colOk, "ON");
        }
        else
        {
            const time_t now = time(0);
            tm nowTm{};
            localtime_s(&nowTm, &now);
            if (isSoundsDateDisabled(&nowTm))
                ImGui::TextColored(colWarn, "OFF  ·  Disabled for Date");
            else
                ImGui::TextColored(colWarn, "OFF  ·  Out of Scheduled Time");
        }
    }

    if (ScriptLibraryFeatureEnabled)
    {
        ScriptLibrary_DrawCurrentContent(item);
    }
    else if (MultiScriptsFeatureEnabled)
    {
        DrawFormLabel("Content", labelCol);
        ImGui::TextDisabled("%s", ShortScriptDisplay(content_filename).c_str());
    }
    else if (WellesleyFeatureEnabled)
    {
        Wellesley_DrawCurrentContent(item);
    }
    else
    {
        DrawFormLabel("Content", labelCol);
        ImGui::Text("%s", ShortScriptDisplay(content_filename).c_str());
        if (ImGui::IsItemHovered() && !content_filename.empty())
            ImGui::SetTooltip("%s", utf8_encode(content_filename).c_str());

        if (HasTwoScripts())
        {
            DrawFormLabel("Client", labelCol);
            ImGui::Text("%s", ShortScriptDisplay(client_filename).c_str());
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", utf8_encode(client_filename).c_str());
        }
    }

    // -----------------------------------------------------------------------
    // Schedule Section
    // -----------------------------------------------------------------------
    DrawSectionHeader("Schedule", "24-hour times written into Schedule.lsc.");

    DrawFormLabel("Start", labelCol);
    createTimeCombo("Scheduled Start Time (Per Day)", start_hour, start_minute);

    if (!g_bUsePerDayEndTimes)
    {
        DrawFormLabel("End", labelCol);
        createTimeCombo("Scheduled End Time (Per Day)", end_hour, end_minute);
        if (scheduleCrossesMidnight(start_hour, start_minute, end_hour, end_minute))
        {
            ImGui::SameLine(0.0f, UIPx(8.0f));
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("next day");
        }
    }

    if (HasTwoScripts())
    {
        char lightsHeader[48];
        std::snprintf(lightsHeader, sizeof(lightsHeader), "Lights Schedule (GPIO %d)", GPIO_PIN_LIGHTS);
        DrawSectionHeader(lightsHeader);

        DrawFormLabel("Start", labelCol);
        createTimeCombo("Scheduled Lights Start Time (Per Day)", lights_start_hour, lights_start_minute);
        DrawFormLabel("End", labelCol);
        createTimeCombo("Scheduled Lights End Time (Per Day)", lights_end_hour, lights_end_minute);
        if (scheduleCrossesMidnight(lights_start_hour, lights_start_minute, lights_end_hour, lights_end_minute))
        {
            ImGui::SameLine(0.0f, UIPx(8.0f));
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("next day");
        }

        char soundsHeader[48];
        std::snprintf(soundsHeader, sizeof(soundsHeader), "Sounds Schedule (GPIO %d)", GPIO_PIN_SOUNDS);
        DrawSectionHeader(soundsHeader);

        DrawFormLabel("Start", labelCol);
        createTimeCombo("Scheduled Sounds Start Time (Per Day)", sounds_start_hour, sounds_start_minute);
        DrawFormLabel("End", labelCol);
        createTimeCombo("Scheduled Sounds End Time (Per Day)", sounds_end_hour, sounds_end_minute);
        if (scheduleCrossesMidnight(sounds_start_hour, sounds_start_minute, sounds_end_hour, sounds_end_minute))
        {
            ImGui::SameLine(0.0f, UIPx(8.0f));
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("next day");
        }

        DrawFormLabel("No-Sound Dates", labelCol);
        if (ImGui::Button("+ Add Date##AddMutedSoundDate"))
        {
            tm newDate{};
            ImGui::SetDateToday(&newDate);
            newDate.tm_hour = 12;
            newDate.tm_isdst = -1;
            std::mktime(&newDate);
            sounds_disabled_dates.push_back(newDate);
        }
        if (!sounds_disabled_dates.empty())
        {
            ImGui::SameLine(0.0f, UIPx(8.0f));
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("(%d configured)", (int)sounds_disabled_dates.size());
        }

        const time_t now = time(0);
        tm todayTm{};
        localtime_s(&todayTm, &now);

        int removeIdx = -1;
        for (int i = 0; i < (int)sounds_disabled_dates.size(); ++i)
        {
            ImGui::PushID(i);
            char rowLabel[32];
            std::snprintf(rowLabel, sizeof(rowLabel), "Date %d", i + 1);
            DrawFormLabel(rowLabel, labelCol);

            ImGui::SetNextItemWidth(UIPx(160.0f));
            if (ImGui::DateChooser("##SoundDisabledDate", sounds_disabled_dates[i], "%d.%m.%Y"))
            {
                sounds_disabled_dates[i].tm_hour = 12;
                sounds_disabled_dates[i].tm_isdst = -1;
                std::mktime(&sounds_disabled_dates[i]);
            }

            ImGui::SameLine(0.0f, UIPx(8.0f));
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.45f, 0.18f, 0.20f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.58f, 0.22f, 0.25f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.68f, 0.26f, 0.28f, 1.00f));
            if (ImGui::Button("Remove"))
            {
                removeIdx = i;
            }
            ImGui::PopStyleColor(3);

            if (isSameCalendarDate(&todayTm, &sounds_disabled_dates[i]))
            {
                ImGui::SameLine(0.0f, UIPx(8.0f));
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(colWarn, "Muted Today");
            }

            ImGui::PopID();
        }
        if (removeIdx >= 0 && removeIdx < (int)sounds_disabled_dates.size())
        {
            sounds_disabled_dates.erase(sounds_disabled_dates.begin() + removeIdx);
        }
    }

    ChooseDays_DrawUI();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    bool bLoadSchedule = ImGui::Button("Load Schedule", ImVec2(UIPx(130.0f), 0.0f));
    ImGui::SameLine(0.0f, UIPx(8.0f));

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.36f, 0.36f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.46f, 0.45f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.29f, 0.62f, 0.60f, 1.00f));
    bool bSaveSchedule = ImGui::Button("Save Schedule", ImVec2(UIPx(130.0f), 0.0f));
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
    ImGui::PopStyleVar(2);
}
