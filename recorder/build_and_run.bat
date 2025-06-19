@echo off
REM Build and run script for recorder.cpp

REM Create images folder if it doesn't exist
if not exist "..\images" mkdir "..\images"

REM Try to use MSVC (cl.exe)
where cl >nul 2>nul
if %errorlevel%==0 (
    echo Compiling with MSVC...
    cl /EHsc /std:c++17 /Fe:recorder.exe recorder.cpp /link gdiplus.lib gdi32.lib user32.lib
    if exist recorder.exe (
        echo Running: recorder.exe (using config.txt)
        recorder.exe
        goto :eof
    )
)

REM Try to use MinGW-w64 (g++)
where g++ >nul 2>nul
if %errorlevel%==0 (
    echo Compiling with MinGW-w64...
    g++ -std=c++17 recorder.cpp -o recorder.exe -lgdiplus -lgdi32 -luser32 -static-libgcc -static-libstdc++
    if exist recorder.exe (
        echo Running: recorder.exe (using config.txt)
        recorder.exe
        goto :eof
    )
)

echo Could not find MSVC (cl.exe) or MinGW-w64 (g++). Please install a Windows C++ compiler. 