#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <iostream>
#include <cwchar>
#include <cstdio>
#include <tchar.h>
#include <vector>
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace fs = std::experimental::filesystem;

#pragma comment (lib, "gdiplus.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "user32.lib")

using namespace Gdiplus;

struct Config {
    int interval_ms;
    int width;
    int height;
    int x;
    int y;
};

Config readConfig(const std::string& filename) {
    Config config = {1000, 800, 600, 0, 0}; // defaults
    std::ifstream file(filename);
    std::string line;
    
    while (std::getline(file, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            if (key == "interval_ms") {
                config.interval_ms = std::stoi(value);
            } else if (key == "width") {
                config.width = std::stoi(value);
            } else if (key == "height") {
                config.height = std::stoi(value);
            } else if (key == "x") {
                config.x = std::stoi(value);
            } else if (key == "y") {
                config.y = std::stoi(value);
            }
        }
    }
    
    return config;
}

int main(int argc, char* argv[]) {
    std::cout << "Starting screen recorder. Press Numpad '*' to start/stop recording.\n" << std::endl;

    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    SetProcessDPIAware();

    RegisterHotKey(NULL, 1, 0, VK_MULTIPLY); // 1 is an arbitrary ID

    MSG msg = {0};
    bool isRecording = false;
    while (true) {
        // Check for hotkey
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_HOTKEY) {
                isRecording = !isRecording;
                if (isRecording) {
                    std::cout << "Recording started. Press Numpad '*' to stop." << std::endl;
                } else {
                    std::cout << "Recording stopped. Press Numpad '*' to start." << std::endl;
                }
            }
        }
        if (isRecording) {
            // Re-query screen size each time
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            std::cout << "Current screen size: " << screenWidth << "x" << screenHeight << std::endl;
            
            // Read config on each iteration for real-time updates
            Config config = readConfig("config.txt");
            
            std::cout << "Using config: interval=" << config.interval_ms 
                      << "ms, width=" << config.width 
                      << ", height=" << config.height
                      << ", x=" << config.x
                      << ", y=" << config.y << std::endl;

            // Use configured x,y coordinates for capture area
            int x = config.x;
            int y = config.y;
            
            // Validate coordinates are within screen bounds
            if (x < 0) x = 0;
            if (y < 0) y = 0;
            if (x + config.width > screenWidth) config.width = screenWidth - x;
            if (y + config.height > screenHeight) config.height = screenHeight - y;

            // Create a device context and compatible bitmap
            HDC hScreenDC = GetDC(NULL);
            HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
            HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, config.width, config.height);
            HGDIOBJ oldBitmap = SelectObject(hMemoryDC, hBitmap);

            // BitBlt from screen to memory DC using x,y coordinates
            BitBlt(hMemoryDC, 0, 0, config.width, config.height, hScreenDC, x, y, SRCCOPY);

            // Save bitmap to file using GDI+
            Bitmap bmp(hBitmap, NULL);
            CLSID pngClsid;
            // Get the CLSID of the PNG encoder
            UINT num = 0, size = 0;
            GetImageEncodersSize(&num, &size);
            if (size == 0) {
                std::cerr << "Failed to get image encoder size.\n";
                return 1;
            }
            ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
            if (pImageCodecInfo == NULL) {
                std::cerr << "Failed to allocate memory for image codec info.\n";
                return 1;
            }
            GetImageEncoders(num, size, pImageCodecInfo);
            for (UINT j = 0; j < num; ++j) {
                if (wcscmp(pImageCodecInfo[j].MimeType, L"image/png") == 0) {
                    pngClsid = pImageCodecInfo[j].Clsid;
                    break;
                }
            }
            free(pImageCodecInfo);

            // Generate unique filename with timestamp
            SYSTEMTIME st;
            GetLocalTime(&st);
            wchar_t filename[256];
            swprintf(filename, 256, L"../images/screenshot_%04d%02d%02d_%02d%02d%02d_%03d.png",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
            bmp.Save(filename, &pngClsid, NULL);

            // Cleanup
            SelectObject(hMemoryDC, oldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hMemoryDC);
            ReleaseDC(NULL, hScreenDC);

            std::wcout << L"Screenshot saved to " << filename << std::endl;

            // Keep only the 5 most recent screenshots
            try {
                std::vector<fs::directory_entry> screenshots;
                for (const auto& entry : fs::directory_iterator("../images")) {
                    if (fs::is_regular_file(entry.path())) {
                        std::wstring fname = entry.path().filename().wstring();
                        if (fname.find(L"screenshot_") == 0 && fname.find(L".png") == fname.length() - 4) {
                            screenshots.push_back(entry);
                        }
                    }
                }
                // Sort by filename (timestamp in name)
                std::sort(screenshots.begin(), screenshots.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
                    return a.path().filename().wstring() < b.path().filename().wstring();
                });
                // Delete oldest if more than 5
                while (screenshots.size() > 5) {
                    fs::remove(screenshots.front().path());
                    screenshots.erase(screenshots.begin());
                }
            } catch (const std::exception& e) {
                std::cerr << "Error managing old screenshots: " << e.what() << std::endl;
            }

            Sleep(config.interval_ms);
        } else {
            Sleep(50); // Sleep briefly to avoid busy-waiting when not recording
        }
    }

    GdiplusShutdown(gdiplusToken);
    return 0;
} 