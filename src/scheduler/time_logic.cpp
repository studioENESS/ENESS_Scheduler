#include "time_logic.h"

#include "app_state.h"
#include "platform.h"
#include "features/feature_csl.h"

bool scheduleCrossesMidnight(int start_hour, int start_minute, int end_hour, int end_minute)
{
    return end_hour < start_hour || (end_hour == start_hour && end_minute < start_minute);
}

static bool isMorningPortion(const tm* localTime, int end_hour, int end_minute)
{
    return localTime->tm_hour < end_hour ||
        (localTime->tm_hour == end_hour && localTime->tm_min <= end_minute);
}

static void resolveEndTime(int dayIndex, int fallback_hour, int fallback_minute, int& out_hour, int& out_minute)
{
    if (g_bUsePerDayEndTimes) {
        out_hour = end_hour_by_day[dayIndex];
        out_minute = end_minute_by_day[dayIndex];
        return;
    }
    out_hour = fallback_hour;
    out_minute = fallback_minute;
}

static int getScheduleDayIndex(const tm* localTime, int start_hour, int start_minute, int end_hour, int end_minute)
{
    int dayToCheck = localTime->tm_wday;
    if (start_hour < 0)
        return dayToCheck;

    if (g_bUsePerDayEndTimes) {
        const int yesterday = (dayToCheck + 6) % 7;
        int yesterdayEndHour = end_hour;
        int yesterdayEndMinute = end_minute;
        resolveEndTime(yesterday, end_hour, end_minute, yesterdayEndHour, yesterdayEndMinute);
        if (scheduleCrossesMidnight(start_hour, start_minute, yesterdayEndHour, yesterdayEndMinute) &&
            isMorningPortion(localTime, yesterdayEndHour, yesterdayEndMinute))
        {
            return yesterday;
        }
        return dayToCheck;
    }

    if (scheduleCrossesMidnight(start_hour, start_minute, end_hour, end_minute) &&
        isMorningPortion(localTime, end_hour, end_minute))
    {
        dayToCheck = (dayToCheck + 6) % 7;
    }
    return dayToCheck;
}

bool IsValidDayOfWeek(int start_hour, int start_minute, int end_hour, int end_minute)
{
    const time_t currentTime = time(0);
    tm localTime;
    localtime_s(&localTime, &currentTime);

    const int dayToCheck = getScheduleDayIndex(&localTime, start_hour, start_minute, end_hour, end_minute);
    return bDays[dayToCheck];
}

bool isDateBetween(tm* time, tm* start, tm* end) {
    // if end time is earlier than the start time, then it is a new day
    if (end->tm_hour < start->tm_hour) {
        end->tm_yday += 1;
    }
    if (time->tm_year < start->tm_year || time->tm_year > end->tm_year) {
        return false;
    }
    if (time->tm_year == start->tm_year && time->tm_year == end->tm_year) {
        if (time->tm_yday < start->tm_yday || time->tm_yday > end->tm_yday) {
            return false;
        }
    }

    return true;
}

bool isTimeInRange(tm* time, int _sh, int _sm, int _eh, int _em)
{
    if (time->tm_hour < _sh || time->tm_hour > _eh) {
        return false;
    }
    if (time->tm_hour == _sh) {
        if (time->tm_min < _sm) {
            return false;
        }
    }

    if (time->tm_hour == _eh) {
        if (time->tm_min > _em) {
            return false;
        }
    }

    return true;
}

bool isTimeBetween(tm* time, int cur_start_hour, int cur_start_minute, int cur_end_hour, int cur_end_minute)
{
    if (!CSL_IsValidDay())
        return false;

    const int dayToCheck = getScheduleDayIndex(time, cur_start_hour, cur_start_minute, cur_end_hour, cur_end_minute);
    int end_hour = cur_end_hour;
    int end_minute = cur_end_minute;
    resolveEndTime(dayToCheck, cur_end_hour, cur_end_minute, end_hour, end_minute);

    // Check if end time is smaller than start time (crosses midnight)
    if (scheduleCrossesMidnight(cur_start_hour, cur_start_minute, end_hour, end_minute)) {
        if ((time->tm_hour > cur_start_hour || (time->tm_hour == cur_start_hour && time->tm_min >= cur_start_minute)) ||
            (time->tm_hour < end_hour || (time->tm_hour == end_hour && time->tm_min <= end_minute))) {
            return true;
        }
        return false;
    }

    // Normal range check
    if (time->tm_hour < cur_start_hour || time->tm_hour > end_hour) {
        return false;
    }
    if (time->tm_hour == cur_start_hour) {
        if (time->tm_min < cur_start_minute) {
            return false;
        }
    }
    if (time->tm_hour == end_hour) {
        if (time->tm_min > end_minute) {
            return false;
        }
    }
    return true;
}
