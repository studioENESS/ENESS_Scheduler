#pragma once

#include "../core/types.h"
#include <GLFW/glfw3.h>
#include "imgui.h"

class GUIManager {
public:
    static bool init(GLFWwindow** window, int width, int height, const char* glsl_version);
    static void cleanup(GLFWwindow* window);
    static void render();
    static void drawMainGUI();

private:
    static void styleColorsPhotoshop();
    static void createTimeCombo(const std::string& comboName, int& currentHour, int& currentMinute);
    static void createScheduleItem(ScheduleItem* item);
    static void pushStyleColors18(float h, bool active = false);
    static void handleFileDialogs();
}; 