#include "feature_audio.h"

#ifdef FEATURE_AUDIO

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>

#include "imgui.h"

#include "../platform.h"
#include "../time_logic.h"

std::vector<SAudioTime> g_vecAudioTimes;

void Audio_LoadJson(nlohmann::json& jsonfile)
{
    if (jsonfile.contains(std::string("audio_times")))
    {
        for (auto& item : jsonfile["audio_times"])
        {
            SAudioTime audioTime;
            audioTime.start_hour = item["start_hour"];
            audioTime.start_minute = item["start_minute"];
            audioTime.end_hour = item["end_hour"];
            audioTime.end_mintute = item["end_minute"];
            audioTime.percentage = item["percentage"];
            g_vecAudioTimes.push_back(audioTime);
        }
    }
}

void Audio_SaveJson(nlohmann::json& jsonfile)
{
    int index = 0;
    for (const auto audioTime : g_vecAudioTimes)
    {
        jsonfile["audio_times"][index]["start_hour"] = audioTime.start_hour;
        jsonfile["audio_times"][index]["start_minute"] = audioTime.start_minute;
        jsonfile["audio_times"][index]["end_hour"] = audioTime.end_hour;
        jsonfile["audio_times"][index]["end_minute"] = audioTime.end_mintute;
        jsonfile["audio_times"][index]["percentage"] = audioTime.percentage;
        index++;
    }
}

static void AddAudioItem(void)
{
    SAudioTime audioTime;

    audioTime.start_hour = 8;
    audioTime.start_minute = 0;
    audioTime.end_hour = 20;
    audioTime.end_mintute = 0;
    audioTime.percentage = 75;
    g_vecAudioTimes.push_back(audioTime);

}

static void createAudioItem(SAudioTime& audio_time, int currentVolume)
{
    ImGui::PushID(&audio_time);
    std::string node_name;
    const time_t currentTime = time(0);
    tm* localTime = new tm();
    localtime_s(localTime, &currentTime);
    if (isTimeInRange(localTime, audio_time.start_hour, audio_time.start_minute, audio_time.end_hour, audio_time.end_mintute))
    {
        node_name.append("* ");
        if (currentVolume != audio_time.percentage)
        {
            std::string sCommand = "amixer -D pulse sset Master " + std::to_string(audio_time.percentage) + "%";
            system(sCommand.c_str());
            currentVolume = audio_time.percentage;
        }
    }
    delete localTime;
    node_name.append("Audio Time: ");
    node_name.append(std::to_string(audio_time.start_hour));
    node_name.append(":");
    node_name.append(std::to_string(audio_time.start_minute));
    node_name.append(" - ");
    node_name.append(std::to_string(audio_time.end_hour));
    node_name.append(":");
    node_name.append(std::to_string(audio_time.end_mintute));
    node_name.append(" - ");
    node_name.append(std::to_string(audio_time.percentage));
    auto index = (intptr_t)&audio_time;
    node_name.append("###").append(std::to_string(index));
    bool node_open = ImGui::TreeNodeEx(node_name.c_str(), ImGuiSelectableFlags_SpanAllColumns);
    if (node_open)
    {
        ImGui::SetNextItemWidth(180);
        if (ImGui::SliderInt("Start Hour", &audio_time.start_hour, 0, 23))
        {
            // do something.
        }

        ImGui::SetNextItemWidth(180);
        if (ImGui::SliderInt("Start Minute", &audio_time.start_minute, 0, 59))
        {
            // do something.
        }

        ImGui::SetNextItemWidth(180);
        if (ImGui::SliderInt("End Hour", &audio_time.end_hour, 0, 23))
        {
            // do something.
        }

        ImGui::SetNextItemWidth(180);
        if (ImGui::SliderInt("End Minute", &audio_time.end_mintute, 0, 59))
        {
            // do something.
        }

        ImGui::SetNextItemWidth(180);
        if (ImGui::SliderInt("Percentage", &audio_time.percentage, 0, 100))
        {
            // do something.
        }

        ImGui::TreePop();

    }

    ImGui::PopID();

}

void Audio_DrawUI()
{
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(32, 0, 128, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(32, 0, 200, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(32, 0, 64, 255));

    bool bAddAudioTime = ImGui::Button("+ Add Audio Time");
    ImGui::PopStyleColor(3);

    if (bAddAudioTime)
    {
        AddAudioItem();
    }
    int currentVolume = -1;
    FILE* pipe = popen("amixer -D pulse sget Master | grep 'Front Left:' | awk -F'[][]' '{ print $2 }'", "r");
    if (pipe) {


        char buffer[128];
        std::string result = "";
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != NULL)
                result += buffer;
        }
        pclose(pipe);


        // Extract volume percentage from the output
        std::istringstream iss(result);
        std::string volume_str;
        while (iss >> volume_str) {
            if (volume_str.back() == '%') {
                volume_str.pop_back(); // Remove the '%' character
                break;
            }
        }

        int volume = std::stoi(volume_str);
        ImGui::Text("Current Volume: %d", volume);
        currentVolume = volume;
    }

    ImGui::BeginChildFrame(3, ImGui::GetContentRegionAvail());
    for (auto& sched : g_vecAudioTimes)
    {
        createAudioItem(sched, currentVolume);

    }

    ImGui::EndChildFrame();
}

#endif // FEATURE_AUDIO
