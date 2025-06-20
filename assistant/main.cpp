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
#pragma comment(lib, "ws2_32.lib")
#include "../recorder/recorder.h"
#include "../matcher/matcher.h"

namespace fs = std::experimental::filesystem;
using namespace Gdiplus;

struct Config {
    int interval_ms;
    int width;
    int height;
    int x;
    int y;
};

Config readConfig(const std::string& filename) {
    Config config = {1000, 800, 600, 0, 0};
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            if (key == "interval_ms") config.interval_ms = std::stoi(value);
            else if (key == "width") config.width = std::stoi(value);
            else if (key == "height") config.height = std::stoi(value);
            else if (key == "x") config.x = std::stoi(value);
            else if (key == "y") config.y = std::stoi(value);
        }
    }
    return config;
}

struct RecordingParams {
    int screenWidth;
    int screenHeight;
    Config config;
};

RecordingParams getRecordingParams() {
    RecordingParams params;
    params.screenWidth = GetSystemMetrics(SM_CXSCREEN);
    params.screenHeight = GetSystemMetrics(SM_CYSCREEN);
    params.config = readConfig("config.txt");
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
    // Parse stream argument
    bool stream = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("--stream=") == 0) {
            std::string val = arg.substr(9);
            stream = (val == "1" || val == "true" || val == "True");
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
                    recParams = getRecordingParams();
                    std::cout << "Recording params: " << recParams.screenWidth << "x" << recParams.screenHeight << std::endl;
                    std::cout << "Config: " << recParams.config.interval_ms << "ms, " << recParams.config.width << "x" << recParams.config.height << " at " << recParams.config.x << "," << recParams.config.y << std::endl;
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
            int x = config.x;
            int y = config.y;
            if (x < 0) x = 0;
            if (y < 0) y = 0;
            if (x + config.width > screenWidth) config.width = screenWidth - x;
            if (y + config.height > screenHeight) config.height = screenHeight - y;

            // Capture screen
            Bitmap* bmp = captureScreen(x, y, config.width, config.height);
            if (bmp) {
                // Call matcher
                matchImage(bmp);
                // Save image if stream
                if (stream) {
                    SYSTEMTIME st;
                    GetLocalTime(&st);
                    wchar_t filename[256];
                    swprintf(filename, 256, L"../images/screenshot_%04d%02d%02d_%02d%02d%02d_%03d.png",
                        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
                    if (saveBitmapToFile(bmp, filename)) {
                        std::wcout << L"Screenshot saved to " << filename << std::endl;
                    }
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
                // Example: send a message for each screenshot
                std::string ts = get_elapsed_timestamp(recording_start);
                send_overlay_message(ts, "#00bfff", "📸 Screenshot taken!");
                delete bmp;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(config.interval_ms));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    GdiplusShutdown(gdiplusToken);
    return 0;
}
