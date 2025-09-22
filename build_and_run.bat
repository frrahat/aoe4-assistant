@echo off
setlocal

REM Parse argument
set stream=0
set run_test=0
if /I "%1"=="test" set run_test=1
if not "%1"=="" if not "%1"=="test" set stream=%1

REM Set OpenCV paths
set OPENCV_DIR=C:\Users\frrah\Downloads\opencv
set OPENCV_INCLUDE=%OPENCV_DIR%\build\include
set OPENCV_LIB=%OPENCV_DIR%\build\x64\vc16\lib
set OPENCV_BIN=%OPENCV_DIR%\build\x64\vc16\bin

REM Add OpenCV DLLs to PATH for runtime
set PATH=%OPENCV_BIN%;%PATH%

REM Build the project
REM Only .cpp files are compiled; .h files are for declarations only
if not exist build mkdir build
pushd build

REM remove .exe files
del /q *.exe >nul 2>&1

REM Clone data folder
xcopy ..\data\ .\data\ /s /e /i /y >nul 2>&1

REM If running test, only build and run the test
if "%1"=="test" (
    cl.exe /EHsc /std:c++17 ../assistant/villager_production_checker/villager_production_checker_test.cpp ../assistant/villager_production_checker/villager_production_checker.cpp ../matcher/matcher.cpp /I../ /I%OPENCV_INCLUDE% /Fe:villager_production_test.exe /link gdiplus.lib user32.lib gdi32.lib %OPENCV_LIB%\opencv_world4110.lib
    villager_production_test.exe

    cl.exe /EHsc /std:c++17 ../assistant/idle_worker_checker/idle_worker_checker_test.cpp ../assistant/idle_worker_checker/idle_worker_checker.cpp ../matcher/matcher.cpp /I../ /I%OPENCV_INCLUDE% /Fe:idle_worker_test.exe /link gdiplus.lib user32.lib gdi32.lib %OPENCV_LIB%\opencv_world4110.lib
    idle_worker_test.exe

    popd
    goto :eof
)

cl.exe /EHsc /std:c++17 ../assistant/main.cpp ../recorder/recorder.cpp ../matcher/matcher.cpp ../assistant/villager_production_checker/villager_production_checker.cpp ../assistant/idle_worker_checker/idle_worker_checker.cpp /I../ /I%OPENCV_INCLUDE% /Fe:aoe4_assistant.exe /link gdiplus.lib user32.lib gdi32.lib %OPENCV_LIB%\opencv_world4110.lib
@REM cl.exe /EHsc /std:c++17 ./overlay/overlay.cpp /I../ /Fe:overlay.exe /link user32.lib gdi32.lib
popd

REM Start the streamer server only if stream is > 0 (run in background)
if %stream% GTR 0 (
    start "" .venv\Scripts\python.exe streamer\server\server.py --stream=%stream%
)

REM Copy the dll file from bin (Necessary when running exe in different env)
copy %OPENCV_BIN%\opencv_world4110.dll .\build\ >nul 2>&1


REM Run the main executable
build\aoe4_assistant.exe --stream=%stream%

REM Usage:
REM   build_and_run.bat           (builds and runs main project)
REM   build_and_run.bat 1         (builds and runs main project with stream 1)
REM   build_and_run.bat test      (builds and runs only the villager production checker test)
