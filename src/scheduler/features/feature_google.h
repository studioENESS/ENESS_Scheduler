// FEATURE_GOOGLE: schedule items with per-item daily start/end times.
// Enable with -DFEATURE_GOOGLE.
#pragma once

#include <ctime>

#include "nlohmann/json.hpp"

#ifdef FEATURE_GOOGLE

inline constexpr bool GoogleFeatureEnabled = true;

void Google_LoadSchedule(nlohmann::json& jsonfile);
void Google_SaveSchedule(nlohmann::json& jsonfile);
// Returns true when it handled the time check for a specific schedule item
// (scheduleItem != -1); false means the caller should use the global times.
bool Google_CheckScheduleItemTime(tm* localTime, int scheduleItem, bool& inTime, bool& validDay);
// "+ Add Schedule Item" button and the schedule item list.
void Google_DrawScheduleUI();
int Google_AdjustWindowHeight(int height);

#else

inline constexpr bool GoogleFeatureEnabled = false;

inline void Google_LoadSchedule(nlohmann::json&) {}
inline void Google_SaveSchedule(nlohmann::json&) {}
inline bool Google_CheckScheduleItemTime(tm*, int, bool&, bool&) { return false; }
inline void Google_DrawScheduleUI() {}
inline int Google_AdjustWindowHeight(int height) { return height; }

#endif // FEATURE_GOOGLE
