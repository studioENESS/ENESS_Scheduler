// Date/time range logic for the scheduler.
#pragma once

#include <ctime>

bool scheduleCrossesMidnight(int start_hour, int start_minute, int end_hour, int end_minute);
bool IsValidDayOfWeek(int start_hour = -1, int start_minute = -1, int end_hour = -1, int end_minute = -1);
bool isDateBetween(tm* time, tm* start, tm* end);
bool isTimeInRange(tm* time, int _sh, int _sm, int _eh, int _em);
bool isTimeBetween(tm* time, int cur_start_hour, int cur_start_minute, int cur_end_hour, int cur_end_minute);
