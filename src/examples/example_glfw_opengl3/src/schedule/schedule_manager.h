#pragma once

#include "../core/types.h"
#include <vector>
#include <memory>

class ScheduleManager {
public:
    static int getCurrentScheduledItem();
    static bool isTimeBetween(tm* time, int curStartHour, int curStartMinute, 
                            int curEndHour, int curEndMinute);
    static bool isDateBetween(tm* time, tm* start, tm* end);
    static void addScheduleItem();
    static void removeScheduleItem(int index);
    static void sortSchedule();

private:
    static std::vector<std::shared_ptr<ScheduleItem>> schedule;
    static int adjustHour(int hour, int modifier);
}; 