#include "platform.h"

#include <cstdio>
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
static pid_t last_pid = 0;
static pid_t client_pid = 0;
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
    if (last_pid != 0)
    {

        kill(last_pid, 1);
        last_pid = 0;
    }
    if (client_pid != 0)
    {
        kill(client_pid, 1);
        client_pid = 0;
    }
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
    std::string cfgPath = utf8_encode(scriptPath);
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
        constexpr const char* kDefaultPlayer = "/home/pi/pixile/player";
        if (pixile_location.empty())
            return kDefaultPlayer;

        const std::string loc = utf8_encode(pixile_location);
        if (!loc.empty() && loc.front() == '/')
        {
            std::string path = loc;
            if (path.back() != '/')
                path += '/';
            path += "player";
            return path;
        }
        return kDefaultPlayer;
    };

    const std::string playerPath = linuxPlayerPath();
    pid_t pid;

    auto launchPlayer = [&](const std::wstring& scriptPath, pid_t& trackedPid) {
        if (trackedPid != 0 || scriptPath.empty())
            return;

        printf("script: %s\n", utf8_encode(scriptPath).c_str());
        pid = fork();
        sleep(1);
        if (pid < 0) {
            perror("fork()");
            return;
        }
        if (pid == 0) {
            const std::string script = utf8_encode(scriptPath);
            const std::string sx = std::to_string(screeninfo.x);
            const std::string sy = std::to_string(screeninfo.y);
            const std::string sw = std::to_string(screeninfo.w);
            const std::string sh = std::to_string(screeninfo.h);
            const std::string sm = std::to_string(g_bUseMouse ? 2 : 0);
            execl(playerPath.c_str(), "player",
                  "-x", sx.c_str(),
                  "-y", sy.c_str(),
                  "-w", sw.c_str(),
                  "-h", sh.c_str(),
                  "-m", sm.c_str(),
                  "-s", script.c_str(),
                  (char*)0);
            perror("execl()");
            exit(0);
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
            pid_t pid;
            int pud_status;
            pid = waitpid(last_pid, &pud_status, WNOHANG);
            if (status == 0) {
                status = PIXILE_STATUS_RUNNING;
            }
            if (pid == -1)
            {
                status = PIXILE_STATUS_OFF;
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
                status = PIXILE_STATUS_OFF;

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
