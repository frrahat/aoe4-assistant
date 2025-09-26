#ifndef SCREENSHOT_TAKER_H
#define SCREENSHOT_TAKER_H

#include <windows.h>
#include <gdiplus.h>
#include <string>

// Initialize GDI+
bool initializeGdiPlus();

// Cleanup GDI+
void cleanupGdiPlus();

// Capture the entire screen and return as a GDI+ Bitmap
Gdiplus::Bitmap* captureFullScreen();

// Save a bitmap to a PNG file
bool saveBitmapToFile(Gdiplus::Bitmap* bitmap, const std::wstring& filename);

// Generate a timestamped filename for the screenshot
std::wstring generateScreenshotFilename();

// Take a screenshot and save it to temp/screenshots directory
bool takeScreenshot();

#endif // SCREENSHOT_TAKER_H
