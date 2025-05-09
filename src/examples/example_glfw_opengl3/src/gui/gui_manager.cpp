#include "gui_manager.h"
#include "../config/config_manager.h"
#include "../process/process_manager.h"
#include "../schedule/schedule_manager.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <portable-file-dialogs.h>
#include <filesystem>

void GUIManager::styleColorsPhotoshop() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.35f, 0.35f, 0.35f, 0.17f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.76f, 0.80f, 0.84f, 0.93f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.60f, 0.73f, 0.88f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.92f, 0.93f, 0.94f, 0.99f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.74f, 0.82f, 0.91f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

void GUIManager::createTimeCombo(const char* label, int* hour, int* minute) {
    ImGui::PushID(label);
    ImGui::BeginGroup();
    ImGui::Text("%s", label);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::BeginCombo("##hour", std::to_string(*hour).c_str())) {
        for (int i = 0; i < 24; i++) {
            if (ImGui::Selectable(std::to_string(i).c_str(), *hour == i)) {
                *hour = i;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::Text(":");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::BeginCombo("##minute", std::to_string(*minute).c_str())) {
        for (int i = 0; i < 60; i++) {
            if (ImGui::Selectable(std::to_string(i).c_str(), *minute == i)) {
                *minute = i;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::EndGroup();
    ImGui::PopID();
}

void GUIManager::createScheduleItem(std::shared_ptr<ScheduleItem>& item) {
    ImGui::PushID(item->index);
    
    if (ImGui::Button("X")) {
        item->deleteMe = true;
    }
    ImGui::SameLine();
    
    ImGui::Text("Item %d", item->index);
    
    if (ImGui::DateChooser("Start Date", &item->startDate)) {
        // Date changed
    }
    
    if (ImGui::DateChooser("End Date", &item->endDate)) {
        // Date changed
    }
    
    if (ConfigManager::getFeatureFlags().google) {
        createTimeCombo("Start Time", &item->start_hour, &item->start_minute);
        createTimeCombo("End Time", &item->end_hour, &item->end_minute);
    }
    
    if (ConfigManager::getFeatureFlags().wellesley) {
        ImGui::Text("Script: %s", utf8_encode(item->script).c_str());
        ImGui::Text("Executable: %s", utf8_encode(item->scriptExecutablePath).c_str());
    }
    
    ImGui::PopID();
}

void GUIManager::handleFileDialog() {
    static std::string lastPath = std::filesystem::current_path().string();
    
    if (ImGui::Button("Load Schedule")) {
        auto dialog = pfd::open_file("Select Schedule File", lastPath,
            { "Schedule Files", "*.lsc", "All Files", "*" });
        if (dialog.result().size() > 0) {
            lastPath = std::filesystem::path(dialog.result()[0]).parent_path().string();
            ConfigManager::loadSchedule(dialog.result()[0]);
        }
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Save Schedule")) {
        auto dialog = pfd::save_file("Save Schedule File", lastPath,
            { "Schedule Files", "*.lsc", "All Files", "*" });
        if (!dialog.result().empty()) {
            lastPath = std::filesystem::path(dialog.result()).parent_path().string();
            ConfigManager::saveSchedule(dialog.result());
        }
    }
}

bool GUIManager::init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsLight();
    styleColorsPhotoshop();
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    
    return true;
}

void GUIManager::cleanup(GLFWwindow* window) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void GUIManager::drawMainGUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    ImGui::Begin("Schedule Manager");
    
    handleFileDialog();
    
    ImGui::Separator();
    
    if (ConfigManager::getFeatureFlags().choose_days) {
        ImGui::Text("Active Days:");
        const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
        for (int i = 0; i < 7; i++) {
            ImGui::SameLine();
            ImGui::Checkbox(days[i], &ConfigManager::getActiveDays()[i]);
        }
    }
    
    ImGui::Separator();
    
    if (ImGui::Button("Add Schedule Item")) {
        ScheduleManager::addScheduleItem();
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Sort Schedule")) {
        ScheduleManager::sortSchedule();
    }
    
    ImGui::Separator();
    
    for (auto& item : ConfigManager::getSchedule()) {
        if (!item->deleteMe) {
            createScheduleItem(item);
            ImGui::Separator();
        }
    }
    
    // Remove deleted items
    auto it = std::remove_if(ConfigManager::getSchedule().begin(), ConfigManager::getSchedule().end(),
        [](const auto& item) { return item->deleteMe; });
    ConfigManager::getSchedule().erase(it, ConfigManager::getSchedule().end());
    
    ImGui::End();
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
} 