// Main scheduler GUI.
#pragma once

#include <string>

void DrawMainGUI();
void StyleColorsPhotoshop();
void SetupFonts(float scale = 1.0f);
void ScaleStyleForDPI(float scale);
float GetUIScale();
float UIPx(float value);
void DrawSectionHeader(const char* title, const char* hint = nullptr);

// Hour/minute combo pair, shared with feature UIs.
void createTimeCombo(std::string sComboName, int& current_hour_idx, int& current_min_idx);
void pushStyleColours18(float h, bool active = false);
