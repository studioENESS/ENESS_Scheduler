#pragma once

#include "../core/types.h"
#include <string>

class ProcessManager {
public:
    static bool killProcess(const std::wstring& processName);
    static bool startPlayer(int programID, const std::wstring& contentFile, 
                          const std::wstring& pixileLocation, const ScreenInfo& screenInfo,
                          bool useMouse);
    static ProcessStatus isProcessRunning(const std::wstring& processName, int scheduleItem);
    static void setCurrentProgram(int programID, int paused = 0);

private:
    static bool killPlayer();
    static bool createProcess(const std::wstring& commandLine, const std::wstring& workingDir);
}; 