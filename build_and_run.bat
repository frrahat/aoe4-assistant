@echo off
setlocal

REM Parse argument
set stream=false
if not "%1"=="" set stream=%1

REM Build the project
REM Only .cpp files are compiled; .h files are for declarations only
if not exist build mkdir build
pushd build
cl.exe /EHsc ../assistant/main.cpp ../recorder/recorder.cpp ../matcher/matcher.cpp /I../ /Fe:aoe4_assistant.exe /link gdiplus.lib user32.lib gdi32.lib
popd

REM Run the main executable
if "%stream%"=="true" (
    build\aoe4_assistant.exe --stream=true
    REM Start the streamer server
    .venv\Scripts\python.exe streamer\server\server.py
) else (
    build\aoe4_assistant.exe --stream=false
)
