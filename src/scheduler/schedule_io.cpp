#include "schedule_io.h"

#include <cstdio>
#include <fstream>
#include <sstream>

#include "nlohmann/json.hpp"

#include "app_state.h"
#include "platform.h"
#include "features/feature_audio.h"
#include "features/feature_choose_days.h"
#include "features/feature_google.h"
#include "features/feature_multiscripts.h"
#include "features/feature_sleep_periods.h"
#include "features/feature_wellesley.h"
#include "features/feature_script_library.h"

std::shared_ptr<pfd::open_file> open_file;
std::shared_ptr<pfd::save_file> save_file;

bool loadSchedule(const char* sFilename)
{
    bool bRes = true;
    std::ifstream file;
    file.open(sFilename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    nlohmann::json jsonfile;
    try {
        jsonfile = nlohmann::json::parse(buffer);
    } catch (const std::exception& e) {
        printf("JSON parse error: %s\n", e.what());
        return false;
    }

    if (jsonfile.contains("content_filename") && jsonfile["content_filename"].is_string())
        orig_content_filename = utf8_decode(jsonfile["content_filename"]);
    content_filename = orig_content_filename;
    if (jsonfile.contains("client_filename") && jsonfile["client_filename"].is_string())
    {
        client_filename = utf8_decode(jsonfile["client_filename"]);
    }

    if (jsonfile.contains("pixile_location") && jsonfile["pixile_location"].is_string())
        pixile_location = utf8_decode(jsonfile["pixile_location"]);

    if (jsonfile.contains("alternate_pixile_location") && jsonfile["alternate_pixile_location"].is_string())
    {
        g_bCanUseAlternatePlayer = true;
        alt_pixile_location = utf8_decode(jsonfile["alternate_pixile_location"]);
        if (jsonfile.contains("use_alternate_player"))
        {
            g_bUseAlternatePlayer = jsonfile["use_alternate_player"].is_boolean() ? jsonfile["use_alternate_player"].get<bool>() : false;
        }
    }

    if (jsonfile.contains("screen") && jsonfile["screen"].is_object())
    {
        auto& screen = jsonfile["screen"];
        screeninfo.x = screen.contains("x") && screen["x"].is_number_integer() ? screen["x"].get<int>() : 0;
        screeninfo.y = screen.contains("y") && screen["y"].is_number_integer() ? screen["y"].get<int>() : 0;
        screeninfo.w = screen.contains("w") && screen["w"].is_number_integer() ? screen["w"].get<int>() : 1024;
        screeninfo.h = screen.contains("h") && screen["h"].is_number_integer() ? screen["h"].get<int>() : 768;
    }

    if (jsonfile.contains("use_mouse"))
    {
        g_bUseMouse = jsonfile["use_mouse"].is_boolean() ? jsonfile["use_mouse"].get<bool>() : false;
    }

    if (jsonfile.contains("time") && jsonfile["time"].is_object()) {
        auto& time = jsonfile["time"];
        if (time.contains("start") && time["start"].is_object()) {
            auto& start = time["start"];
            start_hour = start.contains("hour") && start["hour"].is_number_integer() ? start["hour"].get<int>() : 6;
            start_minute = start.contains("minute") && start["minute"].is_number_integer() ? start["minute"].get<int>() : 55;
        }
        if (time.contains("end") && time["end"].is_object()) {
            auto& end = time["end"];
            end_hour = end.contains("hour") && end["hour"].is_number_integer() ? end["hour"].get<int>() : 20;
            end_minute = end.contains("minute") && end["minute"].is_number_integer() ? end["minute"].get<int>() : 55;
        }
        ChooseDays_LoadJson(time);
    }

    MultiScripts_LoadJson(jsonfile);
    Google_LoadSchedule(jsonfile);
    Wellesley_LoadSchedule(jsonfile);
    ScriptLibrary_LoadSchedule(jsonfile);
    SleepPeriods_LoadJson(jsonfile);
    Audio_LoadJson(jsonfile);

    return bRes;
}

bool saveSchedule(const char* sFilename)
{
    bool bRes = true;
    std::ofstream outfile;
    nlohmann::json jsonfile;
    jsonfile["content_filename"] = utf8_encode(orig_content_filename);
    jsonfile["client_filename"] = utf8_encode(client_filename);
    jsonfile["pixile_location"] = utf8_encode(pixile_location);

    if (g_bCanUseAlternatePlayer)
    {
        jsonfile["alternate_pixile_location"] = utf8_encode(alt_pixile_location);
        jsonfile["use_alternate_player"] = g_bUseAlternatePlayer;
    }

    jsonfile["screen"]["x"] = screeninfo.x;
    jsonfile["screen"]["y"] = screeninfo.y;
    jsonfile["screen"]["w"] = screeninfo.w;
    jsonfile["screen"]["h"] = screeninfo.h;

    jsonfile["use_mouse"] = g_bUseMouse;

    jsonfile["time"]["start"]["hour"] = start_hour;
    jsonfile["time"]["start"]["minute"] = start_minute;
    jsonfile["time"]["end"]["hour"] = end_hour;
    jsonfile["time"]["end"]["minute"] = end_minute;

    ChooseDays_SaveJson(jsonfile);
    Google_SaveSchedule(jsonfile);
    Wellesley_SaveSchedule(jsonfile);
    ScriptLibrary_SaveSchedule(jsonfile);
    MultiScripts_SaveJson(jsonfile);
    Audio_SaveJson(jsonfile);

    outfile.open(sFilename, std::ios::out | std::ios::trunc);

    outfile << jsonfile.dump(4);
    outfile.close();

    return bRes;
}

void DoFileDialog_Open()
{
    if (open_file && open_file->ready())
    {
        auto result = open_file->result();
        if (result.size() != 0)
        {
            loadSchedule(result[0].c_str());

        }
        open_file = nullptr;
    }
}


void DoFileDialog_Save()
{
    if (save_file && save_file->ready())
    {
        auto result = save_file->result();
        if (result.size() != 0)
        {

            saveSchedule(result.c_str());
        }
        save_file = nullptr;
    }
}
