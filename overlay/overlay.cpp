#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <mutex>

// Globals for window position
int g_windowX = 50; // Default: middle left
int g_windowY = 400;

struct OverlayMessage {
    std::wstring timestamp;
    COLORREF color;
    std::wstring text;
};

std::vector<OverlayMessage> g_messages;
std::mutex g_msg_mutex;

COLORREF parseColor(const std::wstring& hex) {
    if (hex.length() != 7 || hex[0] != L'#') return RGB(255,255,255);
    int r = std::stoi(hex.substr(1,2), nullptr, 16);
    int g = std::stoi(hex.substr(3,2), nullptr, 16);
    int b = std::stoi(hex.substr(5,2), nullptr, 16);
    return RGB(r,g,b);
}

void addMessage(const std::wstring& msg) {
    // Format: timestamp|#RRGGBB|text
    size_t p1 = msg.find(L'|');
    size_t p2 = msg.find(L'|', p1+1);
    if (p1 == std::wstring::npos || p2 == std::wstring::npos) return;
    OverlayMessage m;
    m.timestamp = msg.substr(0, p1);
    m.color = parseColor(msg.substr(p1+1, p2-p1-1));
    m.text = msg.substr(p2+1);
    std::lock_guard<std::mutex> lock(g_msg_mutex);
    g_messages.push_back(m);
    if (g_messages.size() > 10) g_messages.erase(g_messages.begin());
}

void tcpServerThread(HWND hwnd) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(50505);
    bind(server, (sockaddr*)&addr, sizeof(addr));
    listen(server, 1);
    while (true) {
        SOCKET client = accept(server, NULL, NULL);
        if (client == INVALID_SOCKET) continue;
        char buf[512];
        int len = recv(client, buf, sizeof(buf)-1, 0);
        if (len > 0) {
            buf[len] = 0;
            int wlen = MultiByteToWideChar(CP_UTF8, 0, buf, len, NULL, 0);
            std::wstring wmsg(wlen, 0);
            MultiByteToWideChar(CP_UTF8, 0, buf, len, &wmsg[0], wlen);
            addMessage(wmsg);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        closesocket(client);
    }
    closesocket(server);
    WSACleanup();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);
        HBRUSH hBrush = CreateSolidBrush(RGB(30, 30, 30));
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);
        SetBkMode(hdc, TRANSPARENT);
        int y = 10;
        std::lock_guard<std::mutex> lock(g_msg_mutex);
        for (const auto& m : g_messages) {
            RECT r = {20, y, rect.right-20, y+30};
            SetTextColor(hdc, RGB(180,180,180));
            DrawTextW(hdc, m.timestamp.c_str(), -1, &r, DT_LEFT|DT_SINGLELINE|DT_NOPREFIX);
            r.left += 70;
            SetTextColor(hdc, m.color);
            DrawTextW(hdc, m.text.c_str(), -1, &r, DT_LEFT|DT_SINGLELINE|DT_NOPREFIX);
            y += 32;
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow) {
    int x = g_windowX, y = g_windowY;
    int width = 400, height = 300;
    int parsed = swscanf(lpCmdLine, L"%d %d", &x, &y);
    if (parsed == 2) { g_windowX = x; g_windowY = y; }
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OverlayWindowClass";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        wc.lpszClassName, L"Chat Overlay",
        WS_POPUP,
        g_windowX, g_windowY, width, height,
        NULL, NULL, hInstance, NULL);
    SetLayeredWindowAttributes(hwnd, 0, 200, LWA_ALPHA);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    std::thread(tcpServerThread, hwnd).detach();
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
} 