#include "time_logic.h"

#include "app_state.h"
#include "platform.h"
#include "features/feature_csl.h"

bool scheduleCrossesMidnight(int start_hour, int start_minute, int end_hour, int end_minute)
{
    return end_hour < start_hour || (end_hour == start_hour && end_minute < start_minute);
}

bool IsValidDayOfWeek(int start_hour, int start_minute, int end_hour, int end_minute)
{
    const time_t currentTime = time(0);
    tm localTime;
    localtime_s(&localTime, &currentTime);

    int dayToCheck = localTime.tm_wday;
    if (start_hour >= 0 && scheduleCrossesMidnight(start_hour, start_minute, end_hour, end_minute))
    {
        const bool inMorningPortion =
            localTime.tm_hour < end_hour ||
            (localTime.tm_hour == end_hour && localTime.tm_min <= end_minute);
        if (inMorningPortion)
            dayToCheck = (dayToCheck + 6) % 7;
    }

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

    // Check if end time is smaller than start time (crosses midnight)
    if (scheduleCrossesMidnight(cur_start_hour, cur_start_minute, cur_end_hour, cur_end_minute)) {
        if ((time->tm_hour > cur_start_hour || (time->tm_hour == cur_start_hour && time->tm_min >= cur_start_minute)) ||
            (time->tm_hour < cur_end_hour || (time->tm_hour == cur_end_hour && time->tm_min <= cur_end_minute))) {
            return true;
        }
        return false;
    }

    // Normal range check
    if (time->tm_hour < cur_start_hour || time->tm_hour > cur_end_hour) {
        return false;
    }
    if (time->tm_hour == cur_start_hour) {
        if (time->tm_min < cur_start_minute) {
            return false;
        }
    }
    if (time->tm_hour == cur_end_hour) {
        if (time->tm_min > cur_end_minute) {
            return false;
        }
    }
    return true;
}
