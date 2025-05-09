#include "schedule_manager.h"
#include "../config/config_manager.h"
#include <ctime>

bool ScheduleManager::isValidDayOfWeek() {
    const auto& flags = ConfigManager::getFeatureFlags();
    if (!flags.choose_days) return true;
    
    time_t currentTime = time(0);
    tm* localTime = new tm();
#ifdef _WIN32
    localtime_s(localTime, &currentTime);
#else
    localtime_r(&currentTime, localTime);
#endif
    
    bool isValid = ConfigManager::getActiveDays()[localTime->tm_wday];
    delete localTime;
    return isValid;
}

bool ScheduleManager::isDateBetween(tm* time, tm* start, tm* end) {
    time_t time_t1 = mktime(time);
    time_t time_t2 = mktime(start);
    time_t time_t3 = mktime(end);
    
    if (time_t1 >= time_t2 && time_t1 <= time_t3) {
        return true;
    }
    
    return false;
}

bool ScheduleManager::isTimeInRange(tm* time, int start_hour, int start_minute, int end_hour, int end_minute) {
    int current_minutes = time->tm_hour * 60 + time->tm_min;
    int start_minutes = start_hour * 60 + start_minute;
    int end_minutes = end_hour * 60 + end_minute;
    
    if (end_minutes < start_minutes) {
        end_minutes += 24 * 60;
        if (current_minutes < start_minutes) {
            current_minutes += 24 * 60;
        }
    }
    
    return current_minutes >= start_minutes && current_minutes <= end_minutes;
}

int ScheduleManager::adjustHour(int hour, int modifier) {
    hour += modifier;
    while (hour >= 24) hour -= 24;
    while (hour < 0) hour += 24;
    return hour;
}

bool ScheduleManager::isTimeBetween(tm* time, int cur_start_hour, int cur_start_minute, int cur_end_hour, int cur_end_minute) {
    const auto& flags = ConfigManager::getFeatureFlags();
    
    if (!flags.choose_days || ScheduleManager::isValidDayOfWeek()) {
        int current_minutes = time->tm_hour * 60 + time->tm_min;
        int start_minutes = cur_start_hour * 60 + cur_start_minute;
        int end_minutes = cur_end_hour * 60 + cur_end_minute;
        
        if (end_minutes < start_minutes) {
            end_minutes += 24 * 60;
            if (current_minutes < start_minutes) {
                current_minutes += 24 * 60;
            }
        }
        
        return current_minutes >= start_minutes && current_minutes <= end_minutes;
    }
    
    return false;
}

int ScheduleManager::getCurrentScheduledItem() {
    const auto& flags = ConfigManager::getFeatureFlags();
    if (!flags.wellesley && !flags.google) return -1;
    
    time_t currentTime = time(0);
    tm* localTime = new tm();
#ifdef _WIN32
    localtime_s(localTime, &currentTime);
#else
    localtime_r(&currentTime, localTime);
#endif
    
    for (const auto& item : ConfigManager::getSchedule()) {
        if (ScheduleManager::isDateBetween(localTime, &item->startDate, &item->endDate)) {
            if (flags.google) {
                if (ScheduleManager::isTimeBetween(localTime, item->start_hour, item->start_minute,
                    item->end_hour, item->end_minute)) {
                    delete localTime;
                    return item->programID;
                }
            } else {
                delete localTime;
                return item->programID;
            }
        }
    }
    
    delete localTime;
    return -1;
}

void ScheduleManager::addScheduleItem() {
    const auto& flags = ConfigManager::getFeatureFlags();
    if (!flags.wellesley && !flags.google) return;
    
    auto newItem = std::make_shared<ScheduleItem>();
    newItem->index = ConfigManager::getSchedule().size();
    
    time_t currentTime = time(0);
    tm* localTime = new tm();
#ifdef _WIN32
    localtime_s(localTime, &currentTime);
#else
    localtime_r(&currentTime, localTime);
#endif
    
    newItem->startDate = *localTime;
    newItem->endDate = *localTime;
    newItem->programID = 0;
    
    if (flags.google) {
        newItem->start_hour = ConfigManager::getStartHour();
        newItem->start_minute = ConfigManager::getStartMinute();
        newItem->end_hour = ConfigManager::getEndHour();
        newItem->end_minute = ConfigManager::getEndMinute();
    }
    
    ConfigManager::getSchedule().push_back(newItem);
    delete localTime;
}

void ScheduleManager::removeScheduleItem(int index) {
    const auto& flags = ConfigManager::getFeatureFlags();
    if (!flags.wellesley && !flags.google) return;
    
    auto& schedule = ConfigManager::getSchedule();
    if (index >= 0 && index < schedule.size()) {
        schedule[index]->deleteMe = true;
    }
}

void ScheduleManager::sortSchedule() {
    const auto& flags = ConfigManager::getFeatureFlags();
    if (!flags.wellesley && !flags.google) return;
    
    auto& schedule = ConfigManager::getSchedule();
    std::sort(schedule.begin(), schedule.end(), [](const auto& a, const auto& b) {
        return std::mktime(&a->startDate) < std::mktime(&b->startDate);
    });
    
    for (size_t i = 0; i < schedule.size(); i++) {
        schedule[i]->index = i;
    }
} 