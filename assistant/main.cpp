#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <gdiplus.h>
#include <iostream>
#include <string>
#include <vector>
#include <experimental/filesystem>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cwchar>
#include <fstream>
#include <sstream>
#pragma comment(lib, "ws2_32.lib")
#include "../recorder/recorder.h"
#include "../matcher/matcher.h"
#include "../third_party/json.hpp"

using json = nlohmann::json;

namespace fs = std::experimental::filesystem;
using namespace Gdiplus;

struct SearchRectangle {
    std::string name;
    int width;
    int height;
    int x;
    int y;
};

struct Config {
    int interval_ms;
    std::vector<SearchRectangle> search_rectangles;
};

Config readConfig(const std::string& filename) {
    Config config;
    config.interval_ms = 1000;
    std::ifstream file(filename);
    if (!file) {
        std::cout << "Config file not found: " << filename << std::endl;
        throw std::runtime_error("Config file not found: " + filename);
    }
    json j;
    file >> j;
    config.interval_ms = j.value("interval_ms", 1000);
    if (j.contains("search_rectangles")) {
        for (const auto& rect : j["search_rectangles"]) {
            SearchRectangle sr;
            sr.name = rect.value("name", "");
            sr.width = rect.value("width", 0);
            sr.height = rect.value("height", 0);
            sr.x = rect.value("x", 0);
            sr.y = rect.value("y", 0);
            config.search_rectangles.push_back(sr);
        }
    }
    return config;
}

struct RecordingParams {
    int screenWidth;
    int screenHeight;
    Config config;
};

RecordingParams getRecordingParams(bool shouldReadFromFile) {
    RecordingParams params;
    params.screenWidth = GetSystemMetrics(SM_CXSCREEN);
    params.screenHeight = GetSystemMetrics(SM_CYSCREEN);
    if (shouldReadFromFile) {
        params.config = readConfig("assistant/config.json");
    } else {
        params.config = Config {1000, {
            SearchRectangle { "villager_production_checker", 300, 36, 11, 733 }
        }};
    }
    return params;
}

bool saveBitmapToFile(Bitmap* bmp, const std::wstring& filename) {
    CLSID pngClsid;
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return false;
    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) return false;
    GetImageEncoders(num, size, pImageCodecInfo);
    bool found = false;
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, L"image/png") == 0) {
            pngClsid = pImageCodecInfo[j].Clsid;
            found = true;
            break;
        }
    }
    free(pImageCodecInfo);
    if (!found) return false;
    return bmp->Save(filename.c_str(), &pngClsid, NULL) == Ok;
}

// Helper to send a message to the overlay
void send_overlay_message(const std::string& timestamp, const std::string& color, const std::string& text) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) return;
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) { WSACleanup(); return; }
    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(50505);
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0) {
        std::string msg = timestamp + "|" + color + "|" + text;
        send(sock, msg.c_str(), (int)msg.size(), 0);
    }
    closesocket(sock);
    WSACleanup();
}

// Helper to get mm:ss timestamp since start
std::string get_elapsed_timestamp(const std::chrono::steady_clock::time_point& start) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
    int minutes = (int)(elapsed / 60);
    int seconds = (int)(elapsed % 60);
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", minutes, seconds);
    return std::string(buf);
}

// Overlay process management
HANDLE g_overlayProcess = NULL;
void start_overlay_process(int x, int y) {
    if (g_overlayProcess) return; // Already running
    std::wstring cmd = L"build\\overlay.exe " + std::to_wstring(x) + L" " + std::to_wstring(y);
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessW(
            NULL,
            &cmd[0],
            NULL, NULL, FALSE,
            CREATE_NO_WINDOW,
            NULL, NULL,
            &si, &pi)) {
        g_overlayProcess = pi.hProcess;
        CloseHandle(pi.hThread);
    }
}
void stop_overlay_process() {
    if (g_overlayProcess) {
        TerminateProcess(g_overlayProcess, 0);
        CloseHandle(g_overlayProcess);
        g_overlayProcess = NULL;
    }
}

int main(int argc, char* argv[]) {
    // Parse stream argument as number
    int stream = 0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("--stream=") == 0) {
            std::string val = arg.substr(9);
            try { stream = std::stoi(val); } catch (...) { stream = 0; }
        }
    }

    std::cout << "Starting screen assistant. Press Numpad '*' to start/stop recording.\n" << std::endl;

    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    SetProcessDPIAware();
    RegisterHotKey(NULL, 1, 0, VK_MULTIPLY); // 1 is an arbitrary ID

    MSG msg = {0};
    bool isRecording = false;
    RecordingParams recParams;
    std::chrono::steady_clock::time_point recording_start;
    while (true) {
        // Check for hotkey
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_HOTKEY) {
                isRecording = !isRecording;
                if (isRecording) {
                    std::cout << "Recording started. Press Numpad '*' to stop." << std::endl;
                    bool shouldReadFromFile = stream > 0;
                    recParams = getRecordingParams(shouldReadFromFile);
                    std::cout << "Config: " << recParams.config.interval_ms << "ms, " << recParams.config.search_rectangles.size() << " rectangles" << std::endl;
                    recording_start = std::chrono::steady_clock::now();
                    start_overlay_process(50, 400);
                    send_overlay_message("00:00", "#ffcc00", "🔥 Recording started!");
                } else {
                    std::cout << "Recording stopped. Press Numpad '*' to start." << std::endl;
                    std::string ts = get_elapsed_timestamp(recording_start);
                    send_overlay_message(ts, "#cccccc", "⏹️ Recording stopped.");
                    stop_overlay_process();
                }
            }
        }
        if (isRecording) {
            int screenWidth = recParams.screenWidth;
            int screenHeight = recParams.screenHeight;
            Config config = recParams.config;
            std::vector<RECT> rects;
            for (const auto& rect : config.search_rectangles) {
                int x = rect.x;
                int y = rect.y;
                int width = rect.width;
                int height = rect.height;
                if (x < 0) x = 0;
                if (y < 0) y = 0;
                if (x + width > screenWidth) width = screenWidth - x;
                if (y + height > screenHeight) height = screenHeight - y;
                RECT r = { x, y, x + width, y + height };
                rects.push_back(r);
            }
            auto bitmaps = captureMultipleRectangles(rects);
            for (size_t i = 0; i < bitmaps.size(); ++i) {
                Bitmap* bmp = bitmaps[i];
                if (bmp) {
                    matchImage(bmp);
                    SYSTEMTIME st;
                    GetLocalTime(&st);
                    wchar_t filename[256];
                    swprintf(filename, 256, L"images/screenshot_%04d%02d%02d_%02d%02d%02d_%03d.png",
                        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
                    if (stream > 0 && stream == static_cast<int>(i+1)) {
                        if (saveBitmapToFile(bmp, filename)) {
                            std::wcout << L"Screenshot saved to " << filename << std::endl;
                        }
                        try {
                            std::vector<fs::directory_entry> screenshots;
                            for (const auto& entry : fs::directory_iterator("images")) {
                                if (fs::is_regular_file(entry.path())) {
                                    std::wstring fname = entry.path().filename().wstring();
                                    if (fname.find(L"screenshot_") == 0 && fname.find(L".png") == fname.length() - 4) {
                                        screenshots.push_back(entry);
                                    }
                                }
                            }
                            std::sort(screenshots.begin(), screenshots.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
                                return a.path().filename().wstring() < b.path().filename().wstring();
                            });
                            while (screenshots.size() > 5) {
                                fs::remove(screenshots.front().path());
                                screenshots.erase(screenshots.begin());
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "Error managing old screenshots: " << e.what() << std::endl;
                        }
                    }
                    std::string ts = get_elapsed_timestamp(recording_start);
                    send_overlay_message(ts, "#00bfff", "📸 Screenshot taken!");
                    delete bmp;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(config.interval_ms));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    GdiplusShutdown(gdiplusToken);
    return 0;
}
