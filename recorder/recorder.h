#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <vector>

Gdiplus::Bitmap* captureScreen(int x, int y, int width, int height);

std::vector<Gdiplus::Bitmap*> captureMultipleRectangles(const std::vector<RECT>& rects); 