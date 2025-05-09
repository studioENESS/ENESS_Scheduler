#include "process_manager.h"
#include "../config/config_manager.h"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

void ProcessManager::killProcessByName(const wchar_t* filename) {
#ifdef _WIN32
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
    PROCESSENTRY32W pEntry;
    pEntry.dwSize = sizeof(pEntry);
    BOOL hRes = Process32FirstW(hSnapShot, &pEntry);
    while (hRes) {
        if (wcscmp(pEntry.szExeFile, filename) == 0) {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0,
                (DWORD)pEntry.th32ProcessID);
            if (hProcess != NULL) {
                TerminateProcess(hProcess, 9);
                CloseHandle(hProcess);
            }
        }
        hRes = Process32NextW(hSnapShot, &pEntry);
    }
    CloseHandle(hSnapShot);
#else
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pkill -f %ls", filename);
    system(cmd);
#endif
}

void ProcessManager::killPlayer() {
#ifdef _WIN32
    killProcessByName(L"player.exe");
    killProcessByName(L"alt_player.exe");
#else
    kill(pid, SIGTERM);
    waitpid(pid, NULL, 0);
#endif
}

void ProcessManager::setCurrentProgram(uint32_t programID, uint32_t paused) {
    nlohmann::json json;
    std::string configPath;
    
    if (ConfigManager::getFeatureFlags().use_hard_paths) {
#ifdef _WIN32
        configPath = "C:\\Content\\config.json";
#else
        configPath = "/home/pi/config.json";
#endif
    } else {
        configPath = "config.json";
    }
    
    std::ifstream file(configPath);
    if (file.is_open()) {
        file >> json;
        file.close();
    }
    
    json["currentProgram"] = programID;
    json["paused"] = paused;
    
    std::ofstream outFile(configPath);
    outFile << json.dump(4);
    outFile.close();
}

void ProcessManager::startPlayer(uint32_t programID) {
    std::string command;
    const auto& flags = ConfigManager::getFeatureFlags();
    
    if (flags.use_hard_paths) {
#ifdef _WIN32
        if (ConfigManager::useAlternatePlayer) {
            command = "C:\\Content\\alt_player.exe";
        } else {
            command = "C:\\Content\\player.exe";
        }
#else
        command = "/home/pi/player";
#endif
    } else {
        if (ConfigManager::useAlternatePlayer) {
            command = "alt_player.exe";
        } else {
            command = "player.exe";
        }
    }
    
    command += " --program " + std::to_string(programID);
    
    if (flags.wellesley) {
        command += " --script " + utf8_encode(ConfigManager::getContentItems()[programID].scriptName);
    }
    
    if (ConfigManager::useMouse) {
        command += " --mouse";
    }
    
    if (ConfigManager::screenInfo.x != 0 || ConfigManager::screenInfo.y != 0) {
        command += " --x " + std::to_string(ConfigManager::screenInfo.x) +
                  " --y " + std::to_string(ConfigManager::screenInfo.y);
    }
    
    if (ConfigManager::screenInfo.w != 1024 || ConfigManager::screenInfo.h != 768) {
        command += " --width " + std::to_string(ConfigManager::screenInfo.w) +
                  " --height " + std::to_string(ConfigManager::screenInfo.h);
    }
    
#ifdef _WIN32
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    std::wstring wCommand = utf8_decode(command);
    CreateProcessW(NULL, (LPWSTR)wCommand.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    pid = fork();
    if (pid == 0) {
        execl(command.c_str(), command.c_str(), NULL);
        exit(1);
    }
#endif
}

ProcessStatus ProcessManager::isProcessRunning(const wchar_t* filename, int programID) {
    if (programID == -1) {
        return ProcessStatus::NOT_SCHEDULED;
    }
    
#ifdef _WIN32
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
    PROCESSENTRY32W pEntry;
    pEntry.dwSize = sizeof(pEntry);
    BOOL hRes = Process32FirstW(hSnapShot, &pEntry);
    while (hRes) {
        if (wcscmp(pEntry.szExeFile, filename) == 0) {
            CloseHandle(hSnapShot);
            return ProcessStatus::RUNNING;
        }
        hRes = Process32NextW(hSnapShot, &pEntry);
    }
    CloseHandle(hSnapShot);
    return ProcessStatus::OFF;
#else
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pgrep -f %ls", filename);
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return ProcessStatus::OFF;
    
    char buffer[128];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    
    return result.empty() ? ProcessStatus::OFF : ProcessStatus::RUNNING;
#endif
} 