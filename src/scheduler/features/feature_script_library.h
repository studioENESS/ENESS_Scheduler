// FEATURE_SCRIPT_LIBRARY: discover .pxl scripts via disk scan and public git
// repos, then pick content/client scripts from a catalog.
// Enable with -DFEATURE_SCRIPT_LIBRARY.
#pragma once

#include "nlohmann/json.hpp"

#ifdef FEATURE_SCRIPT_LIBRARY

#include <cstdint>
#include <string>
#include <vector>

inline constexpr bool ScriptLibraryFeatureEnabled = true;

struct ScriptCatalogEntry
{
    std::string id;
    std::wstring name;
    std::wstring content_path;
    std::wstring client_path;
    std::wstring pixile_location;
    std::string source; // "scan" | "git" | "manual"
    bool name_locked = false;
};

struct ScanRoot
{
    std::wstring path;
    bool enabled = true;
};

struct GitRepoSource
{
    std::string url;
    std::wstring clone_path;
    std::wstring search_subdir;
};

extern std::vector<ScriptCatalogEntry> g_scriptCatalog;
extern std::vector<ScanRoot> g_scanRoots;
extern std::vector<GitRepoSource> g_gitRepos;
extern bool g_gitAvailable;

void ScriptLibrary_Init();
void ScriptLibrary_Save();
void ScriptLibrary_PollFileDialogs();

bool ScriptLibrary_ApplyEntry(const std::string& id);
const ScriptCatalogEntry* ScriptLibrary_FindEntry(const std::string& id);
int ScriptLibrary_FindEntryIndex(const std::string& id);

void ScriptLibrary_LoadSchedule(nlohmann::json& jsonfile);
void ScriptLibrary_SaveSchedule(nlohmann::json& jsonfile);
bool ScriptLibrary_ScriptChanged(int item, int lastItem);
void ScriptLibrary_DrawCurrentContent(int item);
void ScriptLibrary_DrawScheduleUI();
void ScriptLibrary_DrawUI();
int ScriptLibrary_AdjustWindowHeight(int height);

#else

inline constexpr bool ScriptLibraryFeatureEnabled = false;

inline void ScriptLibrary_Init() {}
inline void ScriptLibrary_Save() {}
inline void ScriptLibrary_PollFileDialogs() {}
inline bool ScriptLibrary_ApplyEntry(const std::string&) { return false; }
inline void ScriptLibrary_LoadSchedule(nlohmann::json&) {}
inline void ScriptLibrary_SaveSchedule(nlohmann::json&) {}
inline bool ScriptLibrary_ScriptChanged(int, int) { return false; }
inline void ScriptLibrary_DrawCurrentContent(int) {}
inline void ScriptLibrary_DrawScheduleUI() {}
inline void ScriptLibrary_DrawUI() {}
inline int ScriptLibrary_AdjustWindowHeight(int height) { return height; }

#endif // FEATURE_SCRIPT_LIBRARY
