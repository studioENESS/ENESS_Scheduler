// FEATURE_SLEEP_PERIODS: scheduled sleep breaks (aka prayer times).
// Enable with -DFEATURE_SLEEP_PERIODS.
#pragma once

#include "nlohmann/json.hpp"

#ifdef FEATURE_SLEEP_PERIODS

#include <cstdint>
#include <ctime>
#include <vector>

struct SSleepSchedule
{

    int32_t index;
    uint8_t breaks;
    tm startDate;
    tm endDate;
    int* hours;
    int* minutes;
    int* duration;
};

extern std::vector<SSleepSchedule> g_vecSleepTimes;

void SleepPeriods_LoadJson(nlohmann::json& jsonfile);

#else

inline void SleepPeriods_LoadJson(nlohmann::json&) {}

#endif // FEATURE_SLEEP_PERIODS
