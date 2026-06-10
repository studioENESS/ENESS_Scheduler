#include "feature_sleep_periods.h"

#ifdef FEATURE_SLEEP_PERIODS

std::vector<SSleepSchedule> g_vecSleepTimes;

void SleepPeriods_LoadJson(nlohmann::json& jsonfile)
{
    if (jsonfile.contains(std::string("time\\sleep")))
    {
        for (auto& item : jsonfile["time"]["sleep"])
        {
            auto newItem = new SSleepSchedule;
            newItem->index = (int32_t)g_vecSleepTimes.size();
            newItem->startDate.tm_year = item["StartDate"]["Year"];
            newItem->startDate.tm_mon = item["StartDate"]["Month"];
            newItem->startDate.tm_mday = item["StartDate"]["Day"];
            newItem->startDate.tm_yday = item["StartDate"]["YearDay"];
            newItem->endDate.tm_year = item["EndDate"]["Year"];
            newItem->endDate.tm_mon = item["EndDate"]["Month"];
            newItem->endDate.tm_mday = item["EndDate"]["Day"];
            newItem->endDate.tm_yday = item["EndDate"]["YearDay"];
            newItem->breaks = item["breaks"].size();
            newItem->duration = new int[newItem->breaks];
            newItem->minutes = new int[newItem->breaks];
            newItem->hours = new int[newItem->breaks];
            for (int i = 0; i < newItem->breaks; ++i)
            {
                newItem->minutes[i] = item["breaks"][i]["minute"];
                newItem->hours[i] = item["breaks"][i]["hour"];
                newItem->duration[i] = item["breaks"][i]["duration"];
            }
        }
    }
}

#endif // FEATURE_SLEEP_PERIODS
