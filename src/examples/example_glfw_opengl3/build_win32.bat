@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
@REM Features are enabled with /D flags; adjust DEFINES to taste (e.g. add /DFEATURE_GOOGLE).
@set OUT_DIR=Debug
@set OUT_EXE=example_glfw_opengl3
@set DEFINES=/DFEATURE_CHOOSE_DAYS /DFEATURE_KINETIC /DUSE_HARD_PATHS /DAUTO_RUNNER_ONLY
@set INCLUDES=/I. /I..\.. /I..\..\backends /I..\libs\glfw\include
@set SOURCES=..\..\scheduler\*.cpp ..\..\scheduler\features\*.cpp imguidatechooser.cpp ..\..\backends\imgui_impl_glfw.cpp ..\..\backends\imgui_impl_opengl3.cpp ..\..\imgui*.cpp
@set LIBS=/LIBPATH:..\libs\glfw\lib-vc2010-32 glfw3.lib opengl32.lib gdi32.lib shell32.lib
mkdir %OUT_DIR%
cl /nologo /Zi /MD %DEFINES% %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS%
