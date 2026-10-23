#include "platform.h"

#include <cstdio>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>

#include "nlohmann/json.hpp"

#include "time_logic.h"
#include "features/feature_choose_days.h"
#include "features/feature_csl.h"
#include "features/feature_google.h"
#include "features/feature_wellesley.h"

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#else
#include <cerrno>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
static pid_t last_pid = 0;
static pid_t client_pid = 0;

static std::string TrimAscii(const std::string& input)
{
    size_t start = 0;
    while (start < input.size() && std::isspace((unsigned char)input[start]))
        ++start;
    size_t end = input.size();
    while (end > start && std::isspace((unsigned char)input[end - 1]))
        --end;
    return input.substr(start, end - start);
}

// Pi schedules sometimes store "-s /path/to/script.pxl" in content_filename.
static std::string LinuxScriptPathForPlayer(const std::wstring& scriptPath)
{
    std::string path = TrimAscii(utf8_encode(scriptPath));
    if (path.size() >= 2 && path[0] == '-' && path[1] == 's' &&
        (path.size() == 2 || std::isspace((unsigned char)path[2])))
    {
        path = TrimAscii(path.substr(2));
    }
    return path;
}

static void StopTrackedPlayerPid(pid_t& pid, bool signalSafe)
{
    if (pid == 0)
        return;

    kill(pid, SIGTERM);
    if (signalSafe)
    {
        waitpid(pid, nullptr, WNOHANG);
        if (kill(pid, 0) == 0)
            kill(pid, SIGKILL);
        waitpid(pid, nullptr, WNOHANG);
        pid = 0;
        return;
    }

    for (int i = 0; i < 20; ++i)
    {
        const pid_t result = waitpid(pid, nullptr, WNOHANG);
        if (result == pid || result == -1)
        {
            pid = 0;
            return;
        }
        usleep(50000);
    }

    kill(pid, SIGKILL);
    waitpid(pid, nullptr, 0);
    pid = 0;
}

static void StopTrackedPlayers(bool signalSafe)
{
    StopTrackedPlayerPid(last_pid, signalSafe);
    StopTrackedPlayerPid(client_pid, signalSafe);
}

static void AtexitKillPlayers()
{
    StopTrackedPlayers(false);
}

static void ShutdownSignalHandler(int sig)
{
    StopTrackedPlayers(true);
    signal(sig, SIG_DFL);
    raise(sig);
}

void PlatformInstallShutdownHandlers()
{
    atexit(AtexitKillPlayers);

    struct sigaction sa{};
    sa.sa_handler = ShutdownSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGHUP, &sa, nullptr);
}

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
#include <locale>
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
    StopTrackedPlayers(false);
#endif
    return bResult;
}

static std::string ConfigJsonPathForScript(const std::wstring& scriptPath)
{
#ifdef _WIN32
    std::wstring cfgPath = scriptPath;
    const auto pos = cfgPath.find_last_of(L"/\\");
    if (pos != std::wstring::npos)
        cfgPath.resize(pos);
    else
        cfgPath.clear();
    cfgPath += L"\\config.json";
    return utf8_encode(cfgPath);
#else
    std::string cfgPath = LinuxScriptPathForPlayer(scriptPath);
    const auto pos = cfgPath.find_last_of('/');
    if (pos != std::string::npos)
        cfgPath.resize(pos);
    else
        cfgPath.clear();
    cfgPath += "/config.json";
    return cfgPath;
#endif
}

void SetCurrentProgram(uint32_t programID, uint32_t paused)
{
    if (WellesleyFeatureEnabled)
        Wellesley_ResolveContentScript(programID);

    const std::string cfgFile = ConfigJsonPathForScript(content_filename);
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
        CSL_ApplyPausedState(jsonfile, paused);
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
    auto linuxPlayerPath = []() -> std::string {
        auto resolveCandidate = [](std::string raw) -> std::string {
            raw = TrimAscii(raw);
            if (raw.empty() || raw.front() != '/')
                return {};

            if (access(raw.c_str(), X_OK) == 0)
            {
                std::string withSlash = raw;
                if (withSlash.back() != '/')
                    withSlash += '/';
                const std::string inDir = withSlash + "player";
                if (access(inDir.c_str(), X_OK) == 0)
                    return inDir;
                if (raw.size() >= 7 &&
                    (raw.compare(raw.size() - 6, 6, "player") == 0 ||
                     raw.compare(raw.size() - 6, 6, "Player") == 0))
                {
                    return raw;
                }
            }

            std::string dir = raw;
            if (dir.back() != '/')
                dir += '/';
            for (const char* name : { "player", "Player" })
            {
                const std::string candidate = dir + name;
                if (access(candidate.c_str(), X_OK) == 0)
                    return candidate;
            }
            return {};
        };

        if (!pixile_location.empty())
        {
            const std::string resolved = resolveCandidate(utf8_encode(pixile_location));
            if (!resolved.empty())
                return resolved;
        }

        constexpr const char* kFallbacks[] = {
            "/home/pi/pixile/player",
            "/home/pi/pixile/Player",
            "/home/pi/Pixile/player",
            "/home/pi/Pixile/Player",
            "/home/pi/pixile/Bin/Studio/Release/player",
            "/home/pi/Pixile/Bin/Studio/Release/player",
            "/usr/local/bin/player",
            "/usr/bin/player",
        };
        for (const char* candidate : kFallbacks)
        {
            if (access(candidate, X_OK) == 0)
                return candidate;
        }

        if (!pixile_location.empty())
        {
            std::string loc = TrimAscii(utf8_encode(pixile_location));
            if (!loc.empty() && loc.front() == '/')
            {
                if (loc.size() >= 6 &&
                    (loc.compare(loc.size() - 6, 6, "player") == 0 ||
                     loc.compare(loc.size() - 6, 6, "Player") == 0))
                {
                    return loc;
                }
                if (loc.back() != '/')
                    loc += '/';
                return loc + "player";
            }
        }
        return "/home/pi/pixile/player";
    };

    const std::string playerPath = linuxPlayerPath();
    pid_t pid;

    auto launchPlayer = [&](const std::wstring& scriptPath, pid_t& trackedPid) {
        if (trackedPid != 0 || scriptPath.empty())
            return;

        const std::string script = LinuxScriptPathForPlayer(scriptPath);
        if (script.empty())
            return;

        printf("script: %s\n", script.c_str());
        pid = fork();
        sleep(1);
        if (pid < 0) {
            perror("fork()");
            return;
        }
        if (pid == 0) {
            const std::string sx = std::to_string(screeninfo.x);
            const std::string sy = std::to_string(screeninfo.y);
            const std::string sw = std::to_string(screeninfo.w);
            const std::string sh = std::to_string(screeninfo.h);
            const std::string sm = std::to_string(g_bUseMouse ? 2 : 0);
            execlp(playerPath.c_str(), "player",
                  "-x", sx.c_str(),
                  "-y", sy.c_str(),
                  "-w", sw.c_str(),
                  "-h", sh.c_str(),
                  "-m", sm.c_str(),
                  "-s", script.c_str(),
                  (char*)0);
            fprintf(stderr, "execl(%s): %s\n", playerPath.c_str(), strerror(errno));
            _exit(1);
        }
        trackedPid = pid;
    };

    launchPlayer(content_filename, last_pid);
    launchPlayer(client_filename, client_pid);
#endif
    return true;
}

EPS isProcessRunning(const wchar_t* processName, int scheduleItem)
{
    EPS status = PIXILE_STATUS_OFF;

    const time_t currentTime = time(0);
    tm* localTime = new tm();
    localtime_s(localTime, &currentTime);
    bool inTime = false;
    bool validDay = ChooseDays_IsValidDay(start_hour, start_minute, end_hour, end_minute);

    if (!Google_CheckScheduleItemTime(localTime, scheduleItem, inTime, validDay))
        inTime = isTimeBetween(localTime, start_hour, start_minute, end_hour, end_minute);

    if (inTime && validDay)
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
            int wstatus = 0;
            const pid_t result = waitpid(last_pid, &wstatus, WNOHANG);
            if (result == 0)
                status = PIXILE_STATUS_RUNNING;
            else if (result == last_pid || result == -1)
            {
                status = PIXILE_STATUS_OFF;
                last_pid = 0;
            }
        }
        else
        {
            status = PIXILE_STATUS_OFF;
        }

        if (!client_filename.empty())
        {
            if (client_pid != 0)
            {
                int wstatus = 0;
                const pid_t result = waitpid(client_pid, &wstatus, WNOHANG);
                if (result == client_pid || result == -1)
                {
                    status = PIXILE_STATUS_OFF;
                    client_pid = 0;
                }
            }
            else
            {
                status = PIXILE_STATUS_OFF;
            }
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

namespace {

struct GpioPinState
{
    int pin;
    bool modeConfigured = false;
    int lastValue = -1; // -1 = unknown, 0 = LOW (off), 1 = HIGH (on)
};

GpioPinState g_gpioPins[] = {
    { GPIO_PIN_LIGHTS, false, -1 },
    { GPIO_PIN_SOUNDS, false, -1 },
};

#ifndef _WIN32
bool g_triedStartingPigpiod = false;
#endif

GpioPinState* FindGpioPinState(int pin)
{
    for (auto& entry : g_gpioPins)
    {
        if (entry.pin == pin)
            return &entry;
    }
    return nullptr;
}

} // namespace

void ResetGpioStateCache()
{
    for (auto& entry : g_gpioPins)
    {
        entry.modeConfigured = false;
        entry.lastValue = -1;
    }
}

void SetGpioPinState(int pin, bool on)
{
    const int targetValue = on ? 1 : 0;
    GpioPinState* state = FindGpioPinState(pin);
    if (state && state->lastValue == targetValue)
        return;

#ifdef _WIN32
    printf("GPIO pin %d -> %s (%d)\n", pin, on ? "ON" : "OFF", targetValue);
#else
    if (!state || !state->modeConfigured)
    {
        const std::string modeCmd = "pigs m " + std::to_string(pin) + " w >/dev/null 2>&1";
        int rc = std::system(modeCmd.c_str());
        if (rc != 0 && !g_triedStartingPigpiod)
        {
            g_triedStartingPigpiod = true;
            std::system("sudo -n pigpiod >/dev/null 2>&1 || pigpiod >/dev/null 2>&1");
            rc = std::system(modeCmd.c_str());
        }
        if (state && rc == 0)
            state->modeConfigured = true;
    }

    const std::string writeCmd = "pigs w " + std::to_string(pin) + " " + std::to_string(targetValue) + " >/dev/null 2>&1";
    std::system(writeCmd.c_str());
#endif

    if (state)
        state->lastValue = targetValue;
}

void UpdateGpioSchedules(bool& outLightsOn, bool& outSoundsOn)
{
    if (!HasTwoScripts())
    {
        outLightsOn = false;
        outSoundsOn = false;
        return;
    }

    const time_t currentTime = time(0);
    tm localTime{};
    localtime_s(&localTime, &currentTime);

    outLightsOn = isFixedScheduleActive(&localTime,
                                        lights_start_hour, lights_start_minute,
                                        lights_end_hour, lights_end_minute);
    outSoundsOn = isFixedScheduleActive(&localTime,
                                        sounds_start_hour, sounds_start_minute,
                                        sounds_end_hour, sounds_end_minute);

    SetGpioPinState(GPIO_PIN_LIGHTS, outLightsOn);
    SetGpioPinState(GPIO_PIN_SOUNDS, outSoundsOn);
}
