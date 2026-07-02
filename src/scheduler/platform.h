// Platform layer: process management and string conversion.
// All _WIN32 / posix differences live in this module.
#pragma once

#include <cstdint>
#include <string>

#include "app_state.h"

#ifndef _WIN32
#include <ctime>
#define localtime_s(x,y) localtime_r(y,x)
#define INTERVAL 2
#endif

#ifndef mymax
#define mymax(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef mymin
#define mymin(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifdef _WIN32
#define SCHEDULER_DEFAULT_SCHEDULE_PATH "C:\\Content\\Schedule.lsc"
#define SCHEDULER_SCRIPT_LIBRARY_PATH "C:\\Content\\script_library.json"
#else
#define SCHEDULER_DEFAULT_SCHEDULE_PATH "/home/pi/Schedule.lsc"
#define SCHEDULER_SCRIPT_LIBRARY_PATH "/home/pi/Content/script_library.json"
#endif

// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring& wstr);
// Convert an UTF8 string to a wide Unicode String
std::wstring utf8_decode(const std::string& str);

void killProcessByName(const wchar_t* filename);
bool killPlayer();
void SetCurrentProgram(uint32_t programID, uint32_t paused = 0);
bool startPlayer(uint32_t programID);
EPS isProcessRunning(const wchar_t* processName, int scheduleItem);
