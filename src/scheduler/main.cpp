// ENESS Lumes Scheduler entry point.
// Dear ImGui GLFW + OpenGL3 application shell; the scheduler UI and logic
// live in the sibling modules (ui_main, schedule_io, platform, time_logic)
// and the optional features under features/ (enabled with -DFEATURE_*).

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    int iWWidth = 400;
    int iWHeight = 260;
    iWHeight = Google_AdjustWindowHeight(iWHeight);
    iWHeight = Wellesley_AdjustWindowHeight(iWHeight);
    iWHeight = ScriptLibrary_AdjustWindowHeight(iWHeight);

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
        ScriptLibrary_PollFileDialogs();

    }

    CleanupIMGUI(window);


    return 0;
}
