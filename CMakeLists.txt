cmake_minimum_required(VERSION 3.10)
# set the project name
project(scheduler)
add_compile_definitions(IMGUI_IMPL_OPENGL_ES2)

# ---------------------------------------------------------------------------
# Scheduler features. Toggle from the command line, e.g.:
#   cmake .. -DFEATURE_GOOGLE=ON -DFEATURE_CHOOSE_DAYS=OFF
# ---------------------------------------------------------------------------
option(FEATURE_CHOOSE_DAYS     "Restrict the schedule to selected days of the week" ON)
option(FEATURE_KINETIC         "Kinetic installation calibration data support"      ON)
option(FEATURE_GOOGLE          "Schedule items with per-item daily start/end times" OFF)
option(FEATURE_WELLESLEY       "Wellesley Library content schedule"                 OFF)
option(FEATURE_AUDIO           "Scheduled audio volume changes (audio tweaker)"     OFF)
option(FEATURE_MULTIPLE_SCRIPTS "Multiple named scripts"                            OFF)
option(FEATURE_SCRIPT_LIBRARY  "Script catalog: scan disk and clone public git repos" OFF)
option(FEATURE_SLEEP_PERIODS   "Scheduled sleep breaks (prayer times)"              OFF)
option(FEATURE_CSL             "CSL content on/off + pause controls"                OFF)
option(USE_HARD_PATHS          "Load/save the schedule from a fixed path"           ON)
option(AUTO_RUNNER_ONLY        "Auto runner only mode"                              ON)

foreach(feature
        FEATURE_CHOOSE_DAYS FEATURE_KINETIC FEATURE_GOOGLE FEATURE_WELLESLEY
        FEATURE_AUDIO FEATURE_MULTIPLE_SCRIPTS FEATURE_SCRIPT_LIBRARY FEATURE_SLEEP_PERIODS FEATURE_CSL
        USE_HARD_PATHS AUTO_RUNNER_ONLY)
    if(${feature})
        add_compile_definitions(${feature})
    endif()
endforeach()

set(IMGUI_ROOT "${PROJECT_SOURCE_DIR}/src")
set(IMGUI_BACKENDS "${PROJECT_SOURCE_DIR}/src/backends")
set(PRG_ROOT "${PROJECT_SOURCE_DIR}/src/examples/example_glfw_opengl3")
set(SCHED_ROOT "${PROJECT_SOURCE_DIR}/src/scheduler")

add_executable(scheduler
	${SCHED_ROOT}/main.cpp
	${SCHED_ROOT}/app_state.cpp
	${SCHED_ROOT}/platform.cpp
	${SCHED_ROOT}/time_logic.cpp
	${SCHED_ROOT}/schedule_io.cpp
	${SCHED_ROOT}/ui_main.cpp
	${SCHED_ROOT}/features/feature_audio.cpp
	${SCHED_ROOT}/features/feature_choose_days.cpp
	${SCHED_ROOT}/features/feature_csl.cpp
	${SCHED_ROOT}/features/feature_google.cpp
	${SCHED_ROOT}/features/feature_kinetic.cpp
	${SCHED_ROOT}/features/feature_multiscripts.cpp
	${SCHED_ROOT}/features/feature_script_library.cpp
	${SCHED_ROOT}/features/feature_sleep_periods.cpp
	${SCHED_ROOT}/features/feature_wellesley.cpp
	${SCHED_ROOT}/features/schedule_list_common.cpp
	${PRG_ROOT}/imguidatechooser.cpp
	${IMGUI_ROOT}/imgui.cpp
	${IMGUI_ROOT}/imgui_draw.cpp
	${IMGUI_ROOT}/imgui_widgets.cpp
	${IMGUI_ROOT}/imgui_tables.cpp
	${IMGUI_ROOT}/backends/imgui_impl_opengl3.cpp
	${IMGUI_ROOT}/backends/imgui_impl_glfw.cpp
	${IMGUI_ROOT}/misc/cpp/imgui_stdlib.cpp
)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
target_include_directories(${PROJECT_NAME} PRIVATE ${IMGUI_ROOT} ${IMGUI_BACKENDS} ${PRG_ROOT})

target_link_libraries(${PROJECT_NAME} GLESv2 EGL X11 pthread rt m c dl glfw)
