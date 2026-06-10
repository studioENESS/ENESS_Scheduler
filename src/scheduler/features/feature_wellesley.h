// FEATURE_WELLESLEY: content library with per-item program selection
// (Wellesley Library installation).
// Enable with -DFEATURE_WELLESLEY.
#pragma once

#include <cstdint>

#include "nlohmann/json.hpp"

#ifdef FEATURE_WELLESLEY

#include <string>
#include <vector>

inline constexpr bool WellesleyFeatureEnabled = true;

struct ContentItem {
    std::string name;
    std::string scriptName;
};

extern std::vector<ContentItem> content_items;

void storeContentItemDataToJson(const std::vector<ContentItem>& items, const std::string& filename);
std::vector<ContentItem> loadContentItemDataFromJson(const std::string& filename);

// Loads the content item list at startup.
void Wellesley_Init();
void Wellesley_LoadSchedule(nlohmann::json& jsonfile);
void Wellesley_SaveSchedule(nlohmann::json& jsonfile);
// Points content_filename at the script of the given program; returns true
// when it did so (the caller should then use content_filename).
bool Wellesley_ResolveContentScript(uint32_t programID);
// True when the scheduled script changed between the two items.
bool Wellesley_ScriptChanged(int item, int lastItem);
// "Current Scheduled Content" line.
void Wellesley_DrawCurrentContent(int item);
// "+ Add Schedule Item" button and the schedule item list.
void Wellesley_DrawScheduleUI();
int Wellesley_AdjustWindowHeight(int height);

#else

inline constexpr bool WellesleyFeatureEnabled = false;

inline void Wellesley_Init() {}
inline void Wellesley_LoadSchedule(nlohmann::json&) {}
inline void Wellesley_SaveSchedule(nlohmann::json&) {}
inline bool Wellesley_ResolveContentScript(uint32_t) { return false; }
inline bool Wellesley_ScriptChanged(int, int) { return false; }
inline void Wellesley_DrawCurrentContent(int) {}
inline void Wellesley_DrawScheduleUI() {}
inline int Wellesley_AdjustWindowHeight(int height) { return height; }

#endif // FEATURE_WELLESLEY
