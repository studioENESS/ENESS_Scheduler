// Main scheduler GUI.
#pragma once

#include <string>

void DrawMainGUI();
void StyleColorsPhotoshop();

// Hour/minute combo pair, shared with feature UIs.
void createTimeCombo(std::string sComboName, int& current_hour_idx, int& current_min_idx);
void pushStyleColours18(float h, bool active = false);
