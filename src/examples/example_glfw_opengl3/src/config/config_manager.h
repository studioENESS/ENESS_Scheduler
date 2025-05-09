#pragma once

#include "../core/types.h"
#include <string>
#include <vector>
#include <memory>

class ConfigManager {
public:
    static bool loadSchedule(const std::string& filename);
    static bool saveSchedule(const std::string& filename);
    static void loadContentItems(const std::string& filename);
    static void saveContentItems(const std::string& filename);

    // Feature flags
    static const FeatureFlags& getFeatureFlags() { return featureFlags; }
    static void setFeatureFlags(const FeatureFlags& flags) { featureFlags = flags; }

    // Getters for configuration
    static std::wstring getContentFilename() { return contentFilename; }
    static std::wstring getPixileLocation() { return pixileLocation; }
    static std::wstring getAlternatePixileLocation() { return altPixileLocation; }
    static std::wstring getClientFilename() { return clientFilename; }
    static ScreenInfo getScreenInfo() { return screenInfo; }
    static std::vector<std::shared_ptr<ScheduleItem>> getSchedule() { return schedule; }
    static std::vector<ContentItem> getContentItems() { return contentItems; }
    static bool useAlternatePlayer() { return useAlternatePlayer; }
    static bool canUseAlternatePlayer() { return canUseAlternatePlayer; }
    static bool useMouse() { return useMouse; }
    static int getStartHour() { return startHour; }
    static int getStartMinute() { return startMinute; }
    static int getEndHour() { return endHour; }
    static int getEndMinute() { return endMinute; }
    static std::vector<bool>& getActiveDays() { return activeDays; }

private:
    static FeatureFlags featureFlags;
    static std::wstring contentFilename;
    static std::wstring pixileLocation;
    static std::wstring altPixileLocation;
    static std::wstring clientFilename;
    static ScreenInfo screenInfo;
    static std::vector<std::shared_ptr<ScheduleItem>> schedule;
    static std::vector<ContentItem> contentItems;
    static bool useAlternatePlayer;
    static bool canUseAlternatePlayer;
    static bool useMouse;
    static int startHour;
    static int startMinute;
    static int endHour;
    static int endMinute;
    static std::vector<bool> activeDays;
}; 