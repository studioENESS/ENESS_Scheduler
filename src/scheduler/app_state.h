// Core application state shared across the scheduler modules.
#pragma once

#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

struct SItemSchedule
{
    int32_t index;
    int8_t programID;
    tm startDate;
    tm endDate;
    int start_hour = 6;
    int start_minute = 55;
    int end_hour = 20;
    int end_minute = 55;
    std::wstring sScript;
    std::wstring sScriptLocation;
    std::wstring sScriptExecutablePath;
    std::string catalog_id;
    bool deleteMe = false;
};

enum EPS
{
    PIXILE_STATUS_OFF,
    PIXILE_STATUS_RUNNING,
    PIXILE_STATUS_NOTSCHEDULED
};

struct sScreeninfo
{
    int x;
    int y;
    int w;
    int h;
};

// Custom comparison function for sorting based on startDate
bool compareByStartDate(const SItemSchedule* a, const SItemSchedule* b);

extern std::vector<SItemSchedule*> g_vecSchedule;

extern std::wstring content_filename;
extern std::wstring orig_content_filename;
extern std::wstring pixile_location;
extern std::wstring alt_pixile_location;
extern std::wstring client_filename;

extern int start_hour;
extern int start_minute;
extern int end_hour;
extern int end_minute;

extern bool g_bUseAlternatePlayer;
extern bool g_bCanUseAlternatePlayer;
extern bool g_bUseMouse;

extern sScreeninfo screeninfo;

// Active days of week (Sunday == index 0). Persisted/edited by the
// CHOOSE_DAYS feature, also consulted by the CSL feature.
extern bool bDays[7];

// When true, each day uses its own end time from the arrays below instead of
// the global end_hour/end_minute. Edited by the CHOOSE_DAYS feature.
extern bool g_bUsePerDayEndTimes;
extern int end_hour_by_day[7];
extern int end_minute_by_day[7];
