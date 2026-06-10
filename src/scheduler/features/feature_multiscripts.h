// FEATURE_MULTIPLE_SCRIPTS: a list of named scripts that can be scheduled.
// Enable with -DFEATURE_MULTIPLE_SCRIPTS.
#pragma once

#include "nlohmann/json.hpp"

#ifdef FEATURE_MULTIPLE_SCRIPTS

#include <cstdint>
#include <string>
#include <vector>

inline constexpr bool MultiScriptsFeatureEnabled = true;

struct SItemScript
{
    int32_t index{};
    std::wstring filename;
    std::wstring pixile_location;
    std::wstring name;  // display_name
};

extern std::vector<SItemScript*> g_vecScripts;

void MultiScripts_LoadJson(nlohmann::json& jsonfile);
void MultiScripts_SaveJson(nlohmann::json& jsonfile);
void MultiScripts_DrawUI();

#else

inline constexpr bool MultiScriptsFeatureEnabled = false;

inline void MultiScripts_LoadJson(nlohmann::json&) {}
inline void MultiScripts_SaveJson(nlohmann::json&) {}
inline void MultiScripts_DrawUI() {}

#endif // FEATURE_MULTIPLE_SCRIPTS
