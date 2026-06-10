// FEATURE_AUDIO: scheduled audio volume changes (aka "audio tweaker").
// Enable with -DFEATURE_AUDIO.
#pragma once

#include "nlohmann/json.hpp"

#ifdef FEATURE_AUDIO

#include <vector>

struct SAudioTime
{
    int start_hour;
    int start_minute;
    int end_hour;
    int end_mintute;
    int percentage;
};

extern std::vector<SAudioTime> g_vecAudioTimes;

void Audio_LoadJson(nlohmann::json& jsonfile);
void Audio_SaveJson(nlohmann::json& jsonfile);
// "+ Add Audio Time" button, current volume readout and the audio time list.
void Audio_DrawUI();

#else

inline void Audio_LoadJson(nlohmann::json&) {}
inline void Audio_SaveJson(nlohmann::json&) {}
inline void Audio_DrawUI() {}

#endif // FEATURE_AUDIO
