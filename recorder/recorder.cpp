#include "recorder.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>

using namespace Gdiplus;

// Only keep the captureScreen function
Bitmap* captureScreen(int x, int y, int width, int height) {
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HGDIOBJ oldBitmap = SelectObject(hMemoryDC, hBitmap);

    if (!BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, x, y, SRCCOPY)) {
        SelectObject(hMemoryDC, oldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);
        return nullptr;
    }

    Bitmap* bmp = new Bitmap(hBitmap, NULL);

    SelectObject(hMemoryDC, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    return bmp;
}

std::vector<Bitmap*> captureMultipleRectangles(const std::vector<RECT>& rects) {
    std::vector<Bitmap*> results;
    if (rects.empty()) return results;
    // Compute bounding box
    int minX = rects[0].left, minY = rects[0].top, maxX = rects[0].right, maxY = rects[0].bottom;
    for (const auto& r : rects) {
        if (r.left < minX) minX = r.left;
        if (r.top < minY) minY = r.top;
        if (r.right > maxX) maxX = r.right;
        if (r.bottom > maxY) maxY = r.bottom;
    }
    int width = maxX - minX;
    int height = maxY - minY;
    Bitmap* fullBmp = captureScreen(minX, minY, width, height);
    if (!fullBmp) return results;
    for (const auto& r : rects) {
        int relX = r.left - minX;
        int relY = r.top - minY;
        Gdiplus::Rect subRect(relX, relY, r.right - r.left, r.bottom - r.top);
        Bitmap* subBmp = fullBmp->Clone(subRect, fullBmp->GetPixelFormat());
        results.push_back(subBmp);
    }
    delete fullBmp;
    return results;
} 