#include "screenshot_taker.h"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace Gdiplus;

static ULONG_PTR gdiplusToken = 0;

bool initializeGdiPlus() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::Status status = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    if (status != Gdiplus::Ok) {
        std::cerr << "Failed to initialize GDI+. Status: " << status << std::endl;
        return false;
    }
    
    return true;
}

void cleanupGdiPlus() {
    if (gdiplusToken != 0) {
        GdiplusShutdown(gdiplusToken);
        gdiplusToken = 0;
    }
}

Gdiplus::Bitmap* captureFullScreen() {
    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    std::cout << "Screen dimensions: " << screenWidth << "x" << screenHeight << std::endl;
    
    // Create device contexts
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    
    // Create bitmap
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);
    HGDIOBJ oldBitmap = SelectObject(hMemoryDC, hBitmap);
    
    // Copy screen to bitmap
    BOOL result = BitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY);
    
    if (!result) {
        SelectObject(hMemoryDC, oldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);
        return nullptr;
    }
    
    // Create GDI+ Bitmap from HBITMAP
    Bitmap* bitmap = new Bitmap(hBitmap, NULL);
    
    // Cleanup
    SelectObject(hMemoryDC, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
    
    return bitmap;
}

bool saveBitmapToFile(Bitmap* bitmap, const std::wstring& filename) {
    if (!bitmap) {
        return false;
    }
    
    // Get PNG encoder CLSID
    CLSID pngClsid;
    UINT num = 0, size = 0;
    
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) {
        return false;
    }
    
    Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) {
        return false;
    }
    
    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
    
    bool found = false;
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, L"image/png") == 0) {
            pngClsid = pImageCodecInfo[j].Clsid;
            found = true;
            break;
        }
    }
    
    free(pImageCodecInfo);
    
    if (!found) {
        return false;
    }
    
    // Save the bitmap
    Gdiplus::Status status = bitmap->Save(filename.c_str(), &pngClsid, NULL);
    return status == Gdiplus::Ok;
}

std::wstring generateScreenshotFilename() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    std::wstringstream ss;
    ss << L"..\\temp\\screenshots\\screenshot_"
       << std::setfill(L'0') << std::setw(4) << st.wYear
       << std::setw(2) << st.wMonth
       << std::setw(2) << st.wDay << L"_"
       << std::setw(2) << st.wHour
       << std::setw(2) << st.wMinute
       << std::setw(2) << st.wSecond << L"_"
       << std::setw(3) << st.wMilliseconds
       << L".png";
    
    return ss.str();
}

bool takeScreenshot() {
    Bitmap* bitmap = captureFullScreen();
    if (!bitmap) {
        std::cerr << "Failed to capture screen" << std::endl;
        return false;
    }
    
    std::wstring filename = generateScreenshotFilename();
    bool success = saveBitmapToFile(bitmap, filename);
    
    if (success) {
        std::wcout << L"Screenshot saved to: " << filename << std::endl;
    } else {
        std::wcerr << L"Failed to save screenshot to: " << filename << std::endl;
    }
    
    delete bitmap;
    return success;
}
