#pragma once

#include <string>
#include <vector>
#include <ctime>

// Common enums
enum class ProcessStatus {
    OFF,
    RUNNING,
    NOT_SCHEDULED
};

// Base schedule item structure
struct ScheduleItem {
    int32_t index;
    std::wstring script;
    std::wstring scriptLocation;
    std::wstring scriptExecutablePath;
    tm startDate;
    tm endDate;
    bool deleteMe = false;
};

// Wellesley specific content item
struct ContentItem {
    std::string name;
    std::string scriptName;
};

// Audio time structure
struct AudioTime {
    int start_hour;
    int start_minute;
    int end_hour;
    int end_minute;
    int percentage;
};

// Screen information structure
struct ScreenInfo {
    int x;
    int y;
    int w;
    int h;
}; 