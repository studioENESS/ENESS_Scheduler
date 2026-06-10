// FEATURE_CSL: temporary content on/off + pause controls, with a
// day-of-week gate on the scheduled time window.
// Enable with -DFEATURE_CSL.
#pragma once

#include <cstdint>

#include "nlohmann/json.hpp"

#ifdef FEATURE_CSL

// Gates isTimeBetween on the active day-of-week.
bool CSL_IsValidDay();
// Writes the paused state into the player config json.
void CSL_ApplyPausedState(nlohmann::json& cfg, uint32_t paused);
// Content on/off + pause buttons.
void CSL_DrawUI();

#else

inline bool CSL_IsValidDay() { return true; }
inline void CSL_ApplyPausedState(nlohmann::json&, uint32_t) {}
inline void CSL_DrawUI() {}

#endif // FEATURE_CSL
