#include "feature_csl.h"

#ifdef FEATURE_CSL

#include "imgui.h"

#include "../platform.h"
#include "../time_logic.h"

bool CSL_IsValidDay()
{
    return IsValidDayOfWeek();
}

void CSL_ApplyPausedState(nlohmann::json& cfg, uint32_t paused)
{
    cfg["StateCollections"][1]["Current Selected State"] = paused;
}

void CSL_DrawUI()
{
    static bool bShowingContent = true;
    static bool bPausedContent = false;
    bool bShowContent = false;
    bool bHideContent = false;
    bool bPauseContent = false;
    bool bResumeContent = false;

    ImGui::BeginChildFrame(2, ImGui::GetContentRegionAvail());
    ImGui::Text("Temporally Enable/Disable Content.");
    if (bShowingContent)
    {

        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 128, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(64, 128, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 64, 16, 255));

        bShowContent = ImGui::Button("Content On", ImVec2(200, 60));
        ImGui::PopStyleColor(3);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(128, 0, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(129, 64, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(64, 0, 16, 255));
        bHideContent = ImGui::Button("Content Off", ImVec2(200, 60));
        ImGui::PopStyleColor(3);
    }
    ImGui::SameLine();
    if (!bPausedContent)
    {

        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 128, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(64, 128, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 64, 16, 255));

        bPauseContent = ImGui::Button("Content Normal", ImVec2(200, 60));
        ImGui::PopStyleColor(3);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(128, 0, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(129, 64, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(64, 0, 16, 255));
        bResumeContent = ImGui::Button("Content Paused", ImVec2(200, 60));
        ImGui::PopStyleColor(3);
    }

    if (bShowContent && bShowingContent)
    {
        bShowingContent = false;
    }
    if (bHideContent && !bShowingContent)
    {
        bShowingContent = true;
    }

    if (bResumeContent && bPausedContent)
    {
        bPausedContent = false;
    }
    if (bPauseContent && !bPausedContent)
    {
        bPausedContent = true;
    }

    SetCurrentProgram(bShowingContent, bPausedContent);

    ImGui::EndChildFrame();
}

#endif // FEATURE_CSL
