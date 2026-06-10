// FEATURE_CHOOSE_DAYS: restrict the schedule to selected days of the week.
// Enable with -DFEATURE_CHOOSE_DAYS.
#pragma once

#include "nlohmann/json.hpp"

#ifdef FEATURE_CHOOSE_DAYS

// Reads the "day" array from the "time" json object.
void ChooseDays_LoadJson(nlohmann::json& timeJson);
// Writes the "time"/"day" array into the schedule json.
void ChooseDays_SaveJson(nlohmann::json& jsonfile);
// Day-of-week checkboxes.
void ChooseDays_DrawUI();
// True when today (accounting for overnight schedules) is an active day.
bool ChooseDays_IsValidDay(int start_hour, int start_minute, int end_hour, int end_minute);

#else

inline void ChooseDays_LoadJson(nlohmann::json&) {}
inline void ChooseDays_SaveJson(nlohmann::json&) {}
inline void ChooseDays_DrawUI() {}
inline bool ChooseDays_IsValidDay(int, int, int, int) { return true; }

#endif // FEATURE_CHOOSE_DAYS
