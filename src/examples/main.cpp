// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "nlohmann/json.hpp"

#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#else
#define localtime_s(x,y) localtime_r(y,x)
//#define min std::min
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
pid_t last_pid = 0;
pid_t client_pid =0;

#define INTERVAL 2
#endif
#ifndef mymax
#define mymax(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef mymin
#define mymin(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "imguidatechooser.h"
#include "portable-file-dialogs.h"

#include <time.h>   // mandatory when implementing ImGui::TestDateChooser() yourself in your code
#include <algorithm>
#include <iosfwd>
#include <fstream>
#include <iostream>
#include <filesystem>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif
#pragma comment(linker, "/SUBSYSTEM:Windows /ENTRY:mainCRTStartup")

#define USE_HARD_PATHS 1
#define AUTO_RUNNER_ONLY
#define ENABLE_SLEEP_PERIODS    // aka - prayer times.
#define AUDIO_TWEEKER
#ifdef AUDIO_TWEEKER
struct SAudioTime
{
    int start_hour;
    int start_minute;
    int end_hour;
    int end_mintute;
    int percentage;
};

std::vector<SAudioTime> g_vecAudioTimes;
#endif
#ifdef _WIN32


// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Convert an UTF8 string to a wide Unicode String
std::wstring utf8_decode(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
#else
#include <codecvt>
std::wstring utf8_decode(const std::string& str)
{
    typedef std::codecvt_utf8<wchar_t> convert_typeX;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}

std::string utf8_encode(const std::wstring& str)
{
    typedef std::codecvt_utf8<wchar_t> convert_typeX;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(str);

}

#endif

//#define WELLESLEY
//#define CSL


struct SItemSchedule
{
    int32_t index;
    int8_t programID;
    tm startDate;
    tm endDate;

};

struct SSleepSchedule
{

    int32_t index;
    uint8_t breaks;
    tm startDate;
    tm endDate;
    int* hours;
    int* minutes;
    int* duration;
};

static bool bDays[7];
enum EPS
{
    PIXILE_STATUS_OFF,
    PIXILE_STATUS_RUNNING,
    PIXILE_STATUS_NOTSCHEDULED
};

std::vector<SItemSchedule*> g_vecSchedule;
std::vector<SSleepSchedule> g_vecSleepTimes;
#ifdef WELLESLEY
const char* content_item_names[] = {
    "Under The Sea",
    "New England Winter",
    "New England Fall",
    "Balloons",
    "Painting",
    "Butterflies",
    "Paper Planes",
    "Penguins",
    "Worms",
    "Fireworks",
    "Birds",
    "Rain Umbrella",
    "Nothing Scheduled"
};
#endif

std::wstring content_filename = L"D:\\Eness_Projects\\2038-Wellesley-Library\\Pixile_Sketch\\Packed\\2038-Wellesley_auto.pxl";
std::wstring pixile_location = L"C:\\Eness_Projects\\pixile\\Bin\\Studio\\Release\\";
std::wstring alt_pixile_location = L"C:\\Eness_Projects\\pixile\\Bin\\Studio\\Release\\";
std::wstring client_filename = L"";
static int start_hour = 6;
static int start_minute = 55;
static int end_hour = 20;
static int end_minute = 55;
static bool g_bUseAlternatePlayer = false;
static bool g_bCanUseAlternatePlayer = false;
static bool g_bUseMouse = false;
struct sScreeninfo
{
    int x;
    int y;
    int w;
    int h;
};
static sScreeninfo screeninfo{ 0,0,1024,768 };
static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

std::shared_ptr<pfd::open_file> open_file;
std::shared_ptr<pfd::save_file> save_file;

void killProcessByName(const wchar_t* filename)
{
#ifdef _WIN32
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
    PROCESSENTRY32 pEntry;
    pEntry.dwSize = sizeof(pEntry);
    BOOL hRes = Process32First(hSnapShot, &pEntry);
    while (hRes)
    {
        if (_wcsicmp(pEntry.szExeFile, filename) == 0)
        {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0,
                (DWORD)pEntry.th32ProcessID);
            if (hProcess != NULL)
            {
                TerminateProcess(hProcess, 9);
                CloseHandle(hProcess);
            }
        }
        hRes = Process32Next(hSnapShot, &pEntry);
    }
    CloseHandle(hSnapShot);
#else

#endif
}

bool killPlayer()
{
    bool bResult = false;
#ifdef _WIN32
    killProcessByName(L"Player.exe");
    killProcessByName(L"Pixile.exe");
#else
    if (last_pid != 0)
    {

        kill(last_pid, 1);
        last_pid = 0;
    }
    if (client_pid!= 0)
    {
        kill(client_pid, 1);
        client_pid = 0;
    }
#endif
    return bResult;
}

void SetCurrentProgram(uint32_t programID, uint32_t paused = 0)
{
    std::fstream fs;
    const std::filesystem::path sptPath = content_filename;
#ifdef  _WIN32
    std::wstring cfgFile = (sptPath.parent_path().c_str());
    cfgFile.append(L"\\config.json");
#else
    std::string cfgFile = (sptPath.parent_path().c_str());
    cfgFile.append("\\config.json");
#endif // _WIN32
    std::ifstream file;
    file.open(cfgFile);
    if (file.is_open())
    {
        std::stringstream buffer;
        buffer << file.rdbuf();
        nlohmann::json jsonfile;
        jsonfile = nlohmann::json::parse(buffer);
        file.close();
        jsonfile["StateCollections"][0]["Current Selected State"] = programID;
#ifdef CSL
        jsonfile["StateCollections"][1]["Current Selected State"] = paused;
#endif
        std::ofstream outfile;
        outfile.open(cfgFile, std::ios::out | std::ios::trunc);

        outfile << jsonfile.dump(4);
        outfile.close();
    }
}

bool startPlayer(uint32_t programID)
{
#ifdef _WIN32
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SetCurrentProgram(programID);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::wstring cmdLine = pixile_location;
    if (g_bUseAlternatePlayer)
    {
        cmdLine = alt_pixile_location;

    }

    cmdLine.append(L"player.exe");

    cmdLine.append(L" -x ");
    cmdLine.append(std::to_wstring(screeninfo.x));
    cmdLine.append(L" -y ");
    cmdLine.append(std::to_wstring(screeninfo.y));
    cmdLine.append(L" -w ");
    cmdLine.append(std::to_wstring(screeninfo.w));
    cmdLine.append(L" -h ");
    cmdLine.append(std::to_wstring(screeninfo.h));
    cmdLine.append(L" -m ");
    cmdLine.append(std::to_wstring(g_bUseMouse ? 2 : 0));
    cmdLine.append(L" \"");
    cmdLine.append(content_filename);
    cmdLine.append(L"\"");
    const std::wstring wcmd = cmdLine;
    LPWSTR cmd = cmdLine.data();

    // Start the child process.
    if (!CreateProcess(NULL,   // No module name (use command line)
        cmd,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        g_bUseAlternatePlayer ? alt_pixile_location.c_str() : pixile_location.c_str(),           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        &pi)           // Pointer to PROCESS_INFORMATION structure
        )
    {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    int y, status;
    pid_t pid;
    if (last_pid == 0)
    {
printf("script: %s", utf8_encode(content_filename).c_str());

        pid = fork();
        sleep(1);
        if (pid < 0) {

            /* This is an error */
            perror("fork()");
            return 1;

        }
        else if (pid == 0) {

            /* This is the CHILD */
            execlp("/home/pi/pixile/player", "player", "-w 800", "-h 600", utf8_encode(content_filename).c_str(), (char*)0);

            perror("execlp()");

            /* An exec does NOT return. */
            /* The next lines won't execute unless there's an error with exec. */

            printf("Child %u, parent %u\n", getpid(), getppid());
            exit(0);

        }
        else {
            /* This is the PARENT */
            printf("Parent %u says child PID is %u\n", getpid(), pid);

            last_pid = pid;
        }
    }
    else
    {
    }

    if (client_pid == 0 && client_filename.size() != 0)
    {
        printf("script: %s", utf8_encode(client_filename).c_str());

        pid = fork();
        sleep(1);
        if (pid < 0) {

            /* This is an error */
            perror("fork()");
            return 1;

        }
        else if (pid == 0) {

            /* This is the CHILD */
            execlp("/home/pi/pixile/player", "player", "-w 800", "-h 600", utf8_encode(client_filename).c_str(), (char*)0);

            perror("execlp()");

            /* An exec does NOT return. */
            /* The next lines won't execute unless there's an error with exec. */

            printf("Child %u, parent %u\n", getpid(), getppid());
            exit(0);

        }
        else {
            /* This is the PARENT */
            printf("Parent %u says child PID is %u\n", getpid(), pid);

            client_pid = pid;
        }
    }
    else
    {
    }
#endif
    return true;
}

bool loadSchedule(const char* sFilename)
{
    // TODO: Load Schedule
    bool bRes = true;
    std::ifstream file;
    file.open(sFilename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    nlohmann::json jsonfile;
    jsonfile = nlohmann::json::parse(buffer);
    printf("Test");
    content_filename = utf8_decode(jsonfile["content_filename"]);
    if (jsonfile.contains(std::string("client_filename")))
    {
        client_filename = utf8_decode(jsonfile["client_filename"]);
    }

    pixile_location = utf8_decode(jsonfile["pixile_location"]);
    if (jsonfile.contains(std::string("alternate_pixile_location")))
    {
        g_bCanUseAlternatePlayer = true;
        alt_pixile_location = utf8_decode(jsonfile["alternate_pixile_location"]);
        if (jsonfile.contains(std::string("use_alternate_player")))
        {
            g_bUseAlternatePlayer = jsonfile["use_alternate_player"];
        }
    }

    if (jsonfile.contains(std::string("screen")))
    {
        screeninfo.x = jsonfile["screen"]["x"];
        screeninfo.y = jsonfile["screen"]["y"];
        screeninfo.w = jsonfile["screen"]["w"];
        screeninfo.h = jsonfile["screen"]["h"];
    }

    if (jsonfile.contains(std::string("use_mouse")))
    {
        g_bUseMouse = jsonfile["use_mouse"];
    }

    start_hour = jsonfile["time"]["start"]["hour"];
    start_minute = jsonfile["time"]["start"]["minute"];
    end_hour = jsonfile["time"]["end"]["hour"];
    end_minute = jsonfile["time"]["end"]["minute"];
#ifdef CSL
    for (int i = 0; i < 7; i++)
    {
        //if (jsonfile.contains(std::string("time/day")))
        bDays[i] = jsonfile["time"]["day"][i];
    }
#endif
#ifdef WELLESLEY



    for (auto item : g_vecSchedule)
    {
        delete item;
    }

    g_vecSchedule.clear();

    for (auto& item : jsonfile["schedule"])
    {
        auto newItem = new SItemSchedule;
        newItem->index = (int32_t)g_vecSchedule.size();
        ImGui::SetDateToday(&newItem->startDate);
        ImGui::SetDateToday(&newItem->endDate);
        newItem->index = item["index"];
        newItem->startDate.tm_year = item["StartDate"]["Year"];
        newItem->startDate.tm_mon = item["StartDate"]["Month"];
        newItem->startDate.tm_mday = item["StartDate"]["Day"];
        newItem->startDate.tm_yday = item["StartDate"]["YearDay"];
        newItem->endDate.tm_year = item["EndDate"]["Year"];
        newItem->endDate.tm_mon = item["EndDate"]["Month"];
        newItem->endDate.tm_mday = item["EndDate"]["Day"];
        newItem->endDate.tm_yday = item["EndDate"]["YearDay"];
        newItem->programID = item["ProgramID"];
        g_vecSchedule.push_back(newItem);
    }
#endif // WELLESLEY

#ifdef ENABLE_SLEEP_PERIODS
    if (jsonfile.contains(std::string("time\\sleep")))
    {
        for (auto& item : jsonfile["time"]["sleep"])
        {
            auto newItem = new SSleepSchedule;
            newItem->index = (int32_t)g_vecSleepTimes.size();
            newItem->startDate.tm_year = item["StartDate"]["Year"];
            newItem->startDate.tm_mon = item["StartDate"]["Month"];
            newItem->startDate.tm_mday = item["StartDate"]["Day"];
            newItem->startDate.tm_yday = item["StartDate"]["YearDay"];
            newItem->endDate.tm_year = item["EndDate"]["Year"];
            newItem->endDate.tm_mon = item["EndDate"]["Month"];
            newItem->endDate.tm_mday = item["EndDate"]["Day"];
            newItem->endDate.tm_yday = item["EndDate"]["YearDay"];
            newItem->breaks = item["breaks"].size();
            newItem->duration = new int[newItem->breaks];
            newItem->minutes = new int[newItem->breaks];
            newItem->hours = new int[newItem->breaks];
            for (int i = 0; i < newItem->breaks; ++i)
            {
                newItem->minutes[i] = item["breaks"][i]["minute"];
                newItem->hours[i] = item["breaks"][i]["hour"];
                newItem->duration[i] = item["breaks"][i]["duration"];
            }
        }
    }
    
#endif
    #ifdef AUDIO_TWEEKER
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
    #endif
    return bRes;
}

bool saveSchedule(const char* sFilename)
{
    // TODO: Save Schedule
    bool bRes = true;
    std::ofstream outfile;
    nlohmann::json jsonfile;
    jsonfile["cmdline"] = "player.exe -w 1920 -h 1080 -s c:\\content\\weleslley\\script.pxl";
    jsonfile["content_filename"] = utf8_encode(content_filename);
    jsonfile["client_filename"] =  utf8_encode(client_filename);
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
#ifdef CSL
    for (int i = 0; i < 7; i++)
    {
        jsonfile["time"]["day"][i] = bDays[i];
    }
#endif
#ifdef WELLESLEY

    for (const auto sched : g_vecSchedule)
    {
        jsonfile["schedule"][sched->index]["index"] = sched->index;
        jsonfile["schedule"][sched->index]["StartDate"]["Year"] = sched->startDate.tm_year;
        jsonfile["schedule"][sched->index]["StartDate"]["Month"] = sched->startDate.tm_mon;
        jsonfile["schedule"][sched->index]["StartDate"]["Day"] = sched->startDate.tm_mday;
        jsonfile["schedule"][sched->index]["StartDate"]["YearDay"] = sched->startDate.tm_yday;

        jsonfile["schedule"][sched->index]["EndDate"]["Year"] = sched->endDate.tm_year;
        jsonfile["schedule"][sched->index]["EndDate"]["Month"] = sched->endDate.tm_mon;
        jsonfile["schedule"][sched->index]["EndDate"]["Day"] = sched->endDate.tm_mday;
        jsonfile["schedule"][sched->index]["EndDate"]["YearDay"] = sched->endDate.tm_yday;
        jsonfile["schedule"][sched->index]["ProgramID"] = sched->programID;
    }
#endif
    #ifdef AUDIO_TWEEKER
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
    #endif
    std::string fileName = sFilename;
    outfile.open(sFilename, std::ios::out | std::ios::trunc);

    outfile << jsonfile.dump(4);
    outfile.close();

    return bRes;
}

bool setCurrent(int programIndex = 0)
{
    // TODO: set Current Program.
    bool bRes = true;
    programIndex++;
    return bRes;
}

void DoFileDialog_Open()
{
    if (open_file && open_file->ready())
    {
        auto result = open_file->result();
        if (result.size() != 0)
        {
            //ns::debug::write_line("Opened File %s", result[0].c_str());

            loadSchedule(result[0].c_str());

        }
        //std::cout << "Opened file " << result[0] << "\n";
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
        //std::cout << "Opened file " << result[0] << "\n";
        save_file = nullptr;
    }
}

bool IsValidDayOfWeek()
{
    const time_t currentTime = time(0);
    tm* localTime = new tm();
    localtime_s(localTime, &currentTime);
    bool bValid = false;
    bValid = bDays[localTime->tm_wday];
    delete localTime;
    return bValid;
}

bool isDateBetween(tm* time, tm* start, tm* end) {
    if (time->tm_year < start->tm_year || time->tm_year > end->tm_year) {
        return false;
    }
    if (time->tm_year == start->tm_year && time->tm_year == end->tm_year) {
        if (time->tm_yday < start->tm_yday || time->tm_yday > end->tm_yday) {
            return false;
        }
    }

    return true;
}
bool isTimeInRange(tm* time, int _sh, int _sm, int _eh, int _em)
{
    if (time->tm_hour < _sh || time->tm_hour > _eh) {
        return false;
    }
    if (time->tm_hour == _sh) {
        if (time->tm_min < _sm) {
            return false;
        }
    }

    if (time->tm_hour == _eh) {
        if (time->tm_min > _em) {
            return false;
        }
    }

    return true;
}
bool isTimeBetween(tm* time) {
#ifdef CSL
    if (!IsValidDayOfWeek())
        return false;
#endif
    if (time->tm_hour < start_hour || time->tm_hour > end_hour) {
        return false;
    }
    if (time->tm_hour == start_hour) {
        if (time->tm_min < start_minute) {
            return false;
        }

    }

    if (time->tm_hour == end_hour) {
        if (time->tm_min > end_minute) {
            return false;
        }

    }

    return true;
}

EPS isProcessRunning(const wchar_t* processName)
{
    EPS status = PIXILE_STATUS_OFF;


    const time_t currentTime = time(0);
    tm* localTime = new tm();
    localtime_s(localTime, &currentTime);

    if (isTimeBetween(localTime))
    {

#ifdef _WIN32
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

        if (Process32First(snapshot, &entry))
            while (Process32Next(snapshot, &entry))
                if (!_wcsicmp(entry.szExeFile, processName))
                    status = PIXILE_STATUS_RUNNING;

        CloseHandle(snapshot);
#else

        if (last_pid != 0)
        {
            pid_t pid;
            int pud_status;
            pid = waitpid(last_pid, &pud_status, WNOHANG);
            if (status == 0) {
                status = PIXILE_STATUS_RUNNING;
            }
            if (pid == -1)
            {
                status - PIXILE_STATUS_OFF;
                last_pid = 0;
            }
        }
        else
        {
            status = PIXILE_STATUS_OFF;
        }

        if (client_pid != 0)
        {
            pid_t pid;
            int pud_status;
            pid = waitpid(client_pid, &pud_status, WNOHANG);
            
            if (pid == -1)
            {
                status - PIXILE_STATUS_OFF;
                
                client_pid = 0;
            }
        }
        else
        {
            status = PIXILE_STATUS_OFF;
        }

#endif

        if (localTime)
            delete localTime;
    }
    else {
        if (localTime)
            delete localTime;
        return PIXILE_STATUS_NOTSCHEDULED;
    }

    return status;

}

int GetCurrentScheduledItem()
{
#
    time_t currentTime = time(0);
    tm* localTime = new tm();
    localtime_s(localTime, &currentTime);


    for (auto& item : g_vecSchedule)
    {
        if (isDateBetween(localTime, &item->startDate, &item->endDate))
        {
            if (localTime)
                delete localTime;
            return item->programID;
        }
    }

    if (localTime)
        delete localTime;
    return 12;
}


void createTimeCombo(std::string sComboName, int& current_hour_idx, int& current_min_idx)
{
    ImGui::PushID(sComboName.c_str());
    // Hour
    {
        auto old_str = std::to_string(current_hour_idx);
        auto new_str = std::string(2 - mymin(2, old_str.length()), '0') + old_str;
        const char* combo_preview_value = new_str.c_str();  // Pass in the preview value visible before opening the combo (it could be anything)
        ImGui::SetNextItemWidth(80);
        if (ImGui::BeginCombo("###HourTime", combo_preview_value))
        {
            for (int n = 0; n < 24; n++)
            {

                const bool is_selected = (current_hour_idx == n);
                auto old_str2 = std::to_string(n);
                auto new_str2 = std::string(2 - mymin(2, old_str2.length()), '0') + old_str2;

                if (ImGui::Selectable(new_str2.c_str(), &is_selected))
                {
                    current_hour_idx = n;
                }
                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
    ImGui::SameLine();
    // Hour
    {
        auto old_str = std::to_string(current_min_idx);
        auto new_str = std::string(2 - mymin(2, old_str.length()), '0') + old_str;
        const char* combo_preview_value = new_str.c_str();  // Pass in the preview value visible before opening the combo (it could be anything)
        ImGui::SetNextItemWidth(80);
        if (ImGui::BeginCombo("###MinuteTime", combo_preview_value))
        {

            for (int n = 0; n < 59; n++)
            {

                const bool is_selected = (current_min_idx == n);
                auto old_str3 = std::to_string(n);
                auto new_str3 = std::string(2 - mymin(2, old_str3.length()), '0') + old_str3;

                if (ImGui::Selectable(new_str3.c_str(), &is_selected))
                {
                    current_min_idx = n;
                }
                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
    ImGui::PopID();
}

void pushStyleColours18(float h, bool active = false) {
    while (h > 1) h = h - 1;
    float offset = 0.45f;
    float o = h + offset;
    while (o > 1) o = o - 1;
    //float v = 0.7;
    float v = 0;
    float grey = 0.2f;

    //float s = 0.5;
    float s = 0.0f;

    ImGui::PushStyleColor(ImGuiCol_CheckMark, (ImVec4)ImColor::HSV(h, s, 1.f));

    ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(h, s, 0.35f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, (ImVec4)ImColor::HSV(h, s, 0.35f));

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(h, s, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(h, s, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(h, 0.5f, 0.7f));

    ImGui::PushStyleColor(ImGuiCol_Header, (ImVec4)ImColor::HSV(h, active ? 0.25f : s, active ? 0.35f : v));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, (ImVec4)ImColor::HSV(h, s, v));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, (ImVec4)ImColor::HSV(h, 0.5f, 0.7f));

    ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(h, s, grey));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(h, s, grey));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(h, 0.5f, 0.7f));

    ImGui::PushStyleColor(ImGuiCol_Tab, (ImVec4)ImColor::HSV(h, s, v));
    ImGui::PushStyleColor(ImGuiCol_TabActive, (ImVec4)ImColor::HSV(h, 0.5f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_TabHovered, (ImVec4)ImColor::HSV(h, s, grey));
    ImGui::PushStyleColor(ImGuiCol_TabUnfocused, (ImVec4)ImColor::HSV(h, s, v));
    ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, (ImVec4)ImColor::HSV(h, s, v));
    ImGui::PushStyleColor(ImGuiCol_Separator, (ImVec4)ImColor::HSV(h, 0.5f, 0.7f));
}

void createScheduleItem(SItemSchedule* item)
{
    item->index = item->index;
#ifdef WELLESLEY
    bool bHighlight = false;
    time_t currentTime = time(0);
    tm* localTime = new tm();
    localtime_s(localTime, &currentTime);

    bHighlight = isDateBetween(localTime, &item->startDate, &item->endDate);
    delete localTime;
    ImGui::PushID(item->index);
    std::string node_name;
    node_name.append(bHighlight ? "* " : "").append(content_item_names[item->programID]);
    //node_name.append(std::to_string(item->index));
    node_name.append(" From: ");
    static char startDateText[128]; strftime(startDateText, 128, "%b %d %Y", &item->startDate);
    node_name.append(startDateText);
    node_name.append(" To: ");
    static char endDateText[128]; strftime(endDateText, 128, "%b %d %Y", &item->endDate);
    node_name.append(endDateText);
    if (bHighlight)
    {
        node_name.append(" * ACTIVE");
    }


    node_name.append("###").append(std::to_string(item->index));
    //pushStyleColours18(item->programID / 12.f, bHighlight);
    bool node_open = ImGui::TreeNodeEx(node_name.c_str(), ImGuiSelectableFlags_SpanAllColumns);
    if (node_open)
    {
        ImGui::SetNextItemWidth(180);
        if (ImGui::DateChooser("Scheduled Start Date", item->startDate, "%b %d %Y"))
        {
            // do something.
        }

        ImGui::SetNextItemWidth(180);
        if (ImGui::DateChooser("Scheduled End Date", item->endDate, "%b %d %Y"))
        {
            // do something.
        }



        ImGui::SetNextItemWidth(180);
        if (ImGui::BeginCombo("Program", content_item_names[item->programID]))
        {
            for (int pr = 0; pr < 12; pr++)
            {
                const bool is_selected = (item->programID == pr);

                if (ImGui::Selectable(content_item_names[pr], &is_selected))
                {
                    //current_hour_idx = pr;
                    item->programID = (uint8_t)pr;
                }
                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        //ImGui::Checkbox("Debug Status", &bPixileRunning);
        ImGui::TreePop();

    }

    //ImGui::PopStyleColor(18);

    ImGui::PopID();
#endif
    //   ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

}
void StyleColorsPhotoshop()
{
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
    colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
    colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
    colors[ImGuiCol_TabActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    // colors[ImGuiCol_DockingPreview] = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
    // colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

    style->ChildRounding = 4.0f;
    style->FrameBorderSize = 1.0f;
    style->FrameRounding = 2.0f;
    style->GrabMinSize = 7.0f;
    style->PopupRounding = 2.0f;
    style->ScrollbarRounding = 12.0f;
    style->ScrollbarSize = 13.0f;
    style->TabBorderSize = 1.0f;
    style->TabRounding = 0.0f;
    style->WindowRounding = 4.0f;
}


void AddScheduleItem(bool bAddItem, bool bValidate)
{
    if (bAddItem)
    {
        auto newItem = new SItemSchedule;
        newItem->index = (uint32_t)g_vecSchedule.size();
        ImGui::SetDateToday(&newItem->startDate);
        ImGui::SetDateToday(&newItem->endDate);
        newItem->programID = 0;
        g_vecSchedule.push_back(newItem);
    }

    if (bValidate)
    {
    }
}

void AddAudioItem(void)
{
    SAudioTime audioTime;
    
    audioTime.start_hour = 8;
    audioTime.start_minute = 0;
    audioTime.end_hour = 20;
    audioTime.end_mintute = 0;
    audioTime.percentage = 75;
    g_vecAudioTimes.push_back(audioTime);

}

void createAudioItem(SAudioTime& audio_time, int currentVolume)
{
    ImGui::PushID(&audio_time);
    std::string node_name;
    const time_t currentTime = time(0);
        tm* localTime = new tm();
        localtime_s(localTime, &currentTime);
        if(isTimeInRange(localTime, audio_time.start_hour, audio_time.start_minute, audio_time.end_hour, audio_time.end_mintute) )
        {
            node_name.append("* ");
            if( currentVolume != audio_time.percentage)
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
    int index = (int)&audio_time;
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
void DrawMainGUI()
{
    static int lastItem = -1;
    static int item = 0;
    item = GetCurrentScheduledItem();
    if (item != lastItem)
    {
        SetCurrentProgram(item);
    }
    lastItem = item;
#ifdef IMGUI_HAS_VIEWPORT
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetWorkPos());
    ImGui::SetNextWindowSize(viewport->GetWorkSize());
    ImGui::SetNextWindowViewport(viewport->ID);
#else
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
#endif
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    static float f = 0.0f;
    static int counter = 0;


#ifdef WELLESLEY
    ImGui::Begin("Wellesley Lumes Scheduler", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);
#else
    ImGui::Begin("Pixile Player Scheduler", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);
#endif
    auto running = isProcessRunning(L"player.exe");
    ImGui::Text("Current Player Status:");
    ImGui::SameLine();
    switch (running)
    {
    case PIXILE_STATUS_RUNNING:
        ImGui::TextColored(ImColor(IM_COL32(0, 255, 0, 255)), "Running");
        break;
    case PIXILE_STATUS_NOTSCHEDULED:
        ImGui::TextColored(ImColor(IM_COL32(255, 255, 0, 255)), "Out of Scheduled Time");
        killPlayer();
        break;
    case PIXILE_STATUS_OFF:
    default:
        ImGui::TextColored(ImColor(IM_COL32(255, 0, 0, 255)), "Not Running");
        //Sleep(500);
        startPlayer(item);
        break;
    }



    std::string sPlayerStartStr;
    if (running == PIXILE_STATUS_RUNNING)
    {
        ImGui::SameLine();
        sPlayerStartStr.append("Restart Lumes");

        sPlayerStartStr.append("##StartPlayer");
        if (ImGui::Button(sPlayerStartStr.c_str()))
        {
            killPlayer();
        }

    }
    if (g_bCanUseAlternatePlayer)
    {
        //ImGui::SameLine();
        if (ImGui::Checkbox("Disable People Tracking", &g_bUseAlternatePlayer))
        {
            killPlayer();

        }
    }
#ifdef WELLESLEY
    ImGui::Text("Current Scheduled Content: ");
    ImGui::SameLine();
    ImGui::Text(content_item_names[item]);
#endif


    ImGui::Text("Staring Time"); ImGui::SameLine();
    createTimeCombo("Scheduled Start Time (Per Day)", start_hour, start_minute);

    ImGui::Text("Ending Time"); ImGui::SameLine();
    createTimeCombo("Scheduled End Time (Per Day)", end_hour, end_minute);

#ifdef CSL
    ImGui::Text("Active Days");
    ImGui::Checkbox("Monday", &bDays[1]); ImGui::SameLine();
    ImGui::Checkbox("Tuesday", &bDays[2]); ImGui::SameLine();
    ImGui::Checkbox("Wednesday", &bDays[3]); ImGui::SameLine();
    ImGui::Checkbox("Thursday", &bDays[4]);
    ImGui::Checkbox("Friday", &bDays[5]); ImGui::SameLine();
    ImGui::Checkbox("Saturday", &bDays[6]); ImGui::SameLine();
    ImGui::Checkbox("Sunday", &bDays[0]);
#endif

    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 128, 32, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(64, 128, 32, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 64, 16, 255));

    bool bLoadSchedule = ImGui::Button("Load Schedule");
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(128, 0, 32, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(129, 64, 32, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(64, 0, 16, 255));
    bool bSaveSchedule = ImGui::Button("Save Schedule");
    ImGui::PopStyleColor(3);


#ifdef  WELLESLEY
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(32, 0, 128, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(32, 0, 200, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(32, 0, 64, 255));

    bool bAddItem = ImGui::Button("+ Add Schedule Item");
    ImGui::PopStyleColor(3);

    // ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(32, 128, 128, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(32, 128, 200, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(32, 128, 64, 255));
    bool bValidate = false;// ImGui::Button("Validate Schedule");
    ImGui::PopStyleColor(3);

#endif //  WELLESLEY

#ifdef AUDIO_TWEEKER
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(32, 0, 128, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(32, 0, 200, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(32, 0, 64, 255));

    bool bAddAudioTime = ImGui::Button("+ Add Audio Time");
    ImGui::PopStyleColor(3);
#endif

    if (bLoadSchedule)
    {
#ifdef USE_HARD_PATHS
        
        
#ifdef _WIN32
    loadSchedule("C:\\Content\\Schedule.lsc");
#else
    loadSchedule("/home/pi/Schedule.lsc");
#endif
#else
        std::vector<std::string> filters = { "Lumes Schedule", "*.lsc" };
        open_file = std::make_shared<pfd::open_file>("Choose file", "C:\\", filters);
#endif

    }

    if (bSaveSchedule)
    {
#ifdef USE_HARD_PATHS

#ifdef _WIN32
    saveSchedule("C:\\Content\\Schedule.lsc");
#else
    saveSchedule("/home/pi/Schedule.lsc");
#endif
#else
        std::vector<std::string> filters = { "Lumes Schedule", "*.lsc" };
        save_file = std::make_shared<pfd::save_file>("Choose file", "C:\\", filters);
#endif
    }
#ifdef WELLESLEY
    AddScheduleItem(bAddItem, bValidate);

    ImGui::BeginChildFrame(2, ImGui::GetContentRegionAvail());
    for (auto& sched : g_vecSchedule)
    {
        createScheduleItem(sched);
    }

    ImGui::EndChildFrame();
#endif // WELLESLEY

#ifdef AUDIO_TWEEKER
    if (bAddAudioTime)
    {
        AddAudioItem();
    }
    int currentVolume =-1;
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
     //   std::string volume_str = result.substr(pos - 2, 2);
     //   int volume = std::stoi(volume_str);
        ImGui::Text("Current Volume: %d", volume);
        currentVolume = volume;
    }
    //std::cout << "Current volume: " << volume << "%" << std::endl;
    
    ImGui::BeginChildFrame(3, ImGui::GetContentRegionAvail());
    for (auto& sched : g_vecAudioTimes)
    {
        createAudioItem(sched,currentVolume);
        
    }

    ImGui::EndChildFrame();
#endif
#ifdef CSL
    static bool bShowingContent = true;
    static bool bPausedContent = false;
    bool bShowContent = false;
    bool bHideContent = false;
    bool bPauseContent = false;
    bool bResumeContent = false;

    ImGui::BeginChildFrame(2, ImGui::GetContentRegionAvail());
    ImGui::Text("Temporally Enable/Disable Content.");
    if (bShowingContent)
    {

        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 128, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(64, 128, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 64, 16, 255));

        bShowContent = ImGui::Button("Content On", ImVec2(200, 60));
        ImGui::PopStyleColor(3);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(128, 0, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(129, 64, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(64, 0, 16, 255));
        bHideContent = ImGui::Button("Content Off", ImVec2(200, 60));
        ImGui::PopStyleColor(3);
    }
    ImGui::SameLine();
    if (!bPausedContent)
    {

        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 128, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(64, 128, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 64, 16, 255));

        bPauseContent = ImGui::Button("Content Normal", ImVec2(200, 60));
        ImGui::PopStyleColor(3);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(128, 0, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(129, 64, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(64, 0, 16, 255));
        bResumeContent = ImGui::Button("Content Paused", ImVec2(200, 60));
        ImGui::PopStyleColor(3);
    }

    if (bShowContent && bShowingContent)
    {
        bShowingContent = false;
    }
    if (bHideContent && !bShowingContent)
    {
        bShowingContent = true;
    }

    if (bResumeContent && bPausedContent)
    {
        bPausedContent = false;
    }
    if (bPauseContent && !bPausedContent)
    {
        bPausedContent = true;
    }

    SetCurrentProgram(bShowingContent, bPausedContent);

    ImGui::EndChildFrame();
#endif // CSL
    ImGui::End();
    ImGui::PopStyleVar(1);
}

int InitIMGUI(GLFWwindow** window, int iWWidth, int iWHeight, const char* glsl_version)
{
    *window = glfwCreateWindow(iWWidth, iWHeight, "ENESS Lumes Scheduler", NULL, NULL);

    if (*window == NULL)
        return 1;

    glfwSetWindowPos(*window, 1200, 200);
    glfwMakeContextCurrent(*window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    StyleColorsPhotoshop();
    // Setup Platform/Renderer back ends
    ImGui_ImplGlfw_InitForOpenGL(*window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    return 0;
}

void CleanupIMGUI(GLFWwindow* window)
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

int main(int, char**)
{

#ifdef _WIN32
    loadSchedule("C:\\Content\\Schedule.lsc");
#else
    loadSchedule("/home/pi/Schedule.lsc");
#endif
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    int iWWidth = 400;
    int iWHeight = 260;
#ifdef WELLESLEY
    iWHeight = 400;
#endif
    GLFWwindow* window = nullptr;
    // Create window with graphics context
    auto res = InitIMGUI(&window, iWWidth, iWHeight, glsl_version);
    // Failed to init window.
    if (res == 1)
        return 1;

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        DrawMainGUI();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        DoFileDialog_Open();
        DoFileDialog_Save();

    }

    CleanupIMGUI(window);


    return 0;
}
