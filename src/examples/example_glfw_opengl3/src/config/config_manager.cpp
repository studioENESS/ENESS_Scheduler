#include "config_manager.h"
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <algorithm>

// Static member initialization
FeatureFlags ConfigManager::featureFlags;
std::wstring ConfigManager::contentFilename;
std::wstring ConfigManager::pixileLocation;
std::wstring ConfigManager::altPixileLocation;
std::wstring ConfigManager::clientFilename;
bool ConfigManager::useAlternatePlayer = false;
bool ConfigManager::canUseAlternatePlayer = false;
bool ConfigManager::useMouse = false;
ScreenInfo ConfigManager::screenInfo{0, 0, 1024, 768};
int ConfigManager::startHour = 6;
int ConfigManager::startMinute = 55;
int ConfigManager::endHour = 20;
int ConfigManager::endMinute = 55;
std::vector<std::shared_ptr<ScheduleItem>> ConfigManager::schedule;
std::vector<ContentItem> ConfigManager::contentItems;
std::vector<bool> ConfigManager::activeDays(7, true);

bool ConfigManager::loadSchedule(const std::string& filename) {
    bool result = true;
    std::ifstream file;
    file.open(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    nlohmann::json jsonfile;
    jsonfile = nlohmann::json::parse(buffer);
    
    // Load feature flags
    if (jsonfile.contains("features")) {
        const auto& features = jsonfile["features"];
        featureFlags.wellesley = features.value("wellesley", false);
        featureFlags.google = features.value("google", false);
        featureFlags.csl = features.value("csl", false);
        featureFlags.choose_days = features.value("choose_days", false);
        featureFlags.multiple_scripts = features.value("multiple_scripts", false);
        featureFlags.use_hard_paths = features.value("use_hard_paths", false);
    }
    
    contentFilename = utf8_decode(jsonfile["content_filename"]);
    
    if (jsonfile.contains("client_filename")) {
        clientFilename = utf8_decode(jsonfile["client_filename"]);
    }

    pixileLocation = utf8_decode(jsonfile["pixile_location"]);

    if (jsonfile.contains("alternate_pixile_location")) {
        canUseAlternatePlayer = true;
        altPixileLocation = utf8_decode(jsonfile["alternate_pixile_location"]);
        if (jsonfile.contains("use_alternate_player")) {
            useAlternatePlayer = jsonfile["use_alternate_player"];
        }
    }

    if (jsonfile.contains("screen")) {
        screenInfo.x = jsonfile["screen"]["x"];
        screenInfo.y = jsonfile["screen"]["y"];
        screenInfo.w = jsonfile["screen"]["w"];
        screenInfo.h = jsonfile["screen"]["h"];
    }

    if (jsonfile.contains("use_mouse")) {
        useMouse = jsonfile["use_mouse"];
    }

    startHour = jsonfile["time"]["start"]["hour"];
    startMinute = jsonfile["time"]["start"]["minute"];
    endHour = jsonfile["time"]["end"]["hour"];
    endMinute = jsonfile["time"]["end"]["minute"];

    if (featureFlags.choose_days && jsonfile.contains("active_days")) {
        const auto& days = jsonfile["active_days"];
        for (int i = 0; i < 7; i++) {
            activeDays[i] = days[i];
        }
    }

    if (featureFlags.wellesley || featureFlags.google) {
        schedule.clear();

        for (auto& item : jsonfile["schedule"]) {
            if (item.is_null())
                continue;

            auto newItem = std::make_shared<ScheduleItem>();
            newItem->index = schedule.size();
            newItem->startDate.tm_year = item["StartDate"]["Year"];
            newItem->startDate.tm_mon = item["StartDate"]["Month"];
            newItem->startDate.tm_mday = item["StartDate"]["Day"];
            newItem->startDate.tm_yday = item["StartDate"]["YearDay"];
            newItem->endDate.tm_year = item["EndDate"]["Year"];
            newItem->endDate.tm_mon = item["EndDate"]["Month"];
            newItem->endDate.tm_mday = item["EndDate"]["Day"];
            newItem->endDate.tm_yday = item["EndDate"]["YearDay"];
            
            if (item.contains("ProgramID"))
                newItem->programID = item["ProgramID"];

            if (item.contains("StartTime")) {
                newItem->start_hour = item["StartTime"]["hour"];
                newItem->start_minute = item["StartTime"]["minute"];
            }
            if (item.contains("EndTime")) {
                newItem->end_hour = item["EndTime"]["hour"];
                newItem->end_minute = item["EndTime"]["minute"];
            }

            newItem->script = contentFilename;
            newItem->scriptExecutablePath = pixileLocation;
            if (item.contains("Script")) {
                newItem->script = utf8_decode(item["Script"]);
            }
            if (item.contains("Executable")) {
                newItem->scriptExecutablePath = utf8_decode(item["Executable"]);
            }

            schedule.push_back(newItem);
        }

        std::sort(schedule.begin(), schedule.end(), [](const auto& a, const auto& b) {
            return std::mktime(&a->startDate) < std::mktime(&b->startDate);
        });
    }

    return result;
}

bool ConfigManager::saveSchedule(const std::string& filename) {
    bool result = true;
    std::ofstream outfile;
    nlohmann::json jsonfile;
    
    // Save feature flags
    jsonfile["features"] = {
        {"wellesley", featureFlags.wellesley},
        {"google", featureFlags.google},
        {"csl", featureFlags.csl},
        {"choose_days", featureFlags.choose_days},
        {"multiple_scripts", featureFlags.multiple_scripts},
        {"use_hard_paths", featureFlags.use_hard_paths}
    };
    
    jsonfile["content_filename"] = utf8_encode(contentFilename);
    jsonfile["client_filename"] = utf8_encode(clientFilename);
    jsonfile["pixile_location"] = utf8_encode(pixileLocation);

    if (canUseAlternatePlayer) {
        jsonfile["alternate_pixile_location"] = utf8_encode(altPixileLocation);
        jsonfile["use_alternate_player"] = useAlternatePlayer;
    }

    jsonfile["screen"]["x"] = screenInfo.x;
    jsonfile["screen"]["y"] = screenInfo.y;
    jsonfile["screen"]["w"] = screenInfo.w;
    jsonfile["screen"]["h"] = screenInfo.h;

    jsonfile["use_mouse"] = useMouse;

    jsonfile["time"]["start"]["hour"] = startHour;
    jsonfile["time"]["start"]["minute"] = startMinute;
    jsonfile["time"]["end"]["hour"] = endHour;
    jsonfile["time"]["end"]["minute"] = endMinute;

    if (featureFlags.choose_days) {
        jsonfile["active_days"] = activeDays;
    }

    if (featureFlags.wellesley || featureFlags.google) {
        int index = 0;
        for (const auto& sched : schedule) {
            jsonfile["schedule"][index]["index"] = index;
            jsonfile["schedule"][index]["StartDate"]["Year"] = sched->startDate.tm_year;
            jsonfile["schedule"][index]["StartDate"]["Month"] = sched->startDate.tm_mon;
            jsonfile["schedule"][index]["StartDate"]["Day"] = sched->startDate.tm_mday;
            jsonfile["schedule"][index]["StartDate"]["YearDay"] = sched->startDate.tm_yday;

            jsonfile["schedule"][index]["EndDate"]["Year"] = sched->endDate.tm_year;
            jsonfile["schedule"][index]["EndDate"]["Month"] = sched->endDate.tm_mon;
            jsonfile["schedule"][index]["EndDate"]["Day"] = sched->endDate.tm_mday;
            jsonfile["schedule"][index]["EndDate"]["YearDay"] = sched->endDate.tm_yday;
            
            jsonfile["schedule"][index]["ProgramID"] = sched->programID;
            
            if (featureFlags.google) {
                jsonfile["schedule"][index]["StartTime"]["hour"] = sched->start_hour;
                jsonfile["schedule"][index]["StartTime"]["minute"] = sched->start_minute;
                jsonfile["schedule"][index]["EndTime"]["hour"] = sched->end_hour;
                jsonfile["schedule"][index]["EndTime"]["minute"] = sched->end_minute;
            }
            
            jsonfile["schedule"][index]["Script"] = utf8_encode(sched->script);
            jsonfile["schedule"][index]["Executable"] = utf8_encode(sched->scriptExecutablePath);
            index++;
        }
    }

    outfile.open(filename, std::ios::out | std::ios::trunc);
    outfile << jsonfile.dump(4);
    outfile.close();

    return result;
}

void ConfigManager::loadContentItems(const std::string& filename) {
    if (!featureFlags.wellesley) return;
    
    std::ifstream inputFile(filename);
    if (inputFile.is_open()) {
        nlohmann::json jsonData;
        inputFile >> jsonData;

        contentItems.clear();
        for (const auto& item : jsonData) {
            ContentItem newItem;
            newItem.name = item["name"];
            newItem.scriptName = item["scriptName"];
            contentItems.push_back(newItem);
        }

        inputFile.close();
    }
}

void ConfigManager::saveContentItems(const std::string& filename) {
    if (!featureFlags.wellesley) return;
    
    nlohmann::json jsonData;
    for (const auto& item : contentItems) {
        jsonData.push_back({
            {"name", item.name},
            {"scriptName", item.scriptName}
        });
    }

    std::ofstream outputFile(filename);
    outputFile << std::setw(4) << jsonData << std::endl;
    outputFile.close();
} 