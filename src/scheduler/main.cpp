// ENESS Lumes Scheduler entry point.
// Dear ImGui GLFW + OpenGL3 application shell; the scheduler UI and logic
// live in the sibling modules (ui_main, schedule_io, platform, time_logic)
// and the optional features under features/ (enabled with -DFEATURE_*).

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif
#ifdef _MSC_VER
#pragma comment(linker, "/SUBSYSTEM:Windows /ENTRY:mainCRTStartup")
#endif

#include "app_state.h"
#include "platform.h"
#include "schedule_io.h"
#include "ui_main.h"
#include "features/feature_google.h"
#include "features/feature_wellesley.h"
#include "features/feature_script_library.h"

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static void EnablePerMonitorDPI()
{
#ifdef _WIN32
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32)
    {
        using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
        auto fn = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (fn && fn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
            return;
    }
    SetProcessDPIAware();
#endif
}

static float QueryMonitorScale()
{
#ifdef _WIN32
    HDC hdc = GetDC(NULL);
    if (hdc)
    {
        int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(NULL, hdc);
        if (dpi > 0)
            return (float)dpi / 96.0f;
    }
#endif
    return 1.0f;
}

int InitIMGUI(GLFWwindow** window, int iWWidth, int iWHeight, const char* glsl_version, float dpiScale)
{
    *window = glfwCreateWindow(iWWidth, iWHeight, "ENESS Lumes Scheduler", NULL, NULL);

    if (*window == NULL)
        return 1;

    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = primary ? glfwGetVideoMode(primary) : nullptr;
    if (mode && mode->width > 0 && mode->height > 0)
    {
        const int px = mode->width > iWWidth ? (mode->width - iWWidth) / 2 : 40;
        const int py = mode->height > iWHeight ? (mode->height - iWHeight) / 2 : 40;
        glfwSetWindowPos(*window, px, py);
    }
    else
    {
        glfwSetWindowPos(*window, 120, 80);
    }

    glfwMakeContextCurrent(*window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    StyleColorsPhotoshop();
    SetupFonts(dpiScale);
    ScaleStyleForDPI(dpiScale);

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
    PlatformInstallShutdownHandlers();
    EnablePerMonitorDPI();

    loadSchedule(SCHEDULER_DEFAULT_SCHEDULE_PATH);

    Wellesley_Init();
    ScriptLibrary_Init();

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
#endif

    const float dpiScale = QueryMonitorScale();

    int baseWidth = 540;
    int baseHeight = 360;
    baseHeight = Google_AdjustWindowHeight(baseHeight);
    baseHeight = Wellesley_AdjustWindowHeight(baseHeight);
    baseHeight = ScriptLibrary_AdjustWindowHeight(baseHeight);
    const int baseWindowHeight = baseHeight;
    if (HasTwoScripts())
        baseHeight += 210;

    int iWWidth = (int)(baseWidth * dpiScale);
    int iWHeight = (int)(baseHeight * dpiScale);

    GLFWwindow* window = nullptr;
    // Create window with graphics context
    auto res = InitIMGUI(&window, iWWidth, iWHeight, glsl_version, dpiScale);
    // Failed to init window.
    if (res == 1)
        return 1;

    ImVec4 clear_color = ImVec4(0.11f, 0.12f, 0.14f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (HasTwoScripts())
        {
            const int minTwoScriptH = (int)((baseWindowHeight + 210) * GetUIScale());
            int curW = 0, curH = 0;
            glfwGetWindowSize(window, &curW, &curH);
            if (curH < minTwoScriptH)
                glfwSetWindowSize(window, curW, minTwoScriptH);
        }

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
        ScriptLibrary_PollFileDialogs();
    }

    killPlayer();
    CleanupIMGUI(window);

    return 0;
}
