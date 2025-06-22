#pragma once
#include <string>
#include <windows.h>
#include <gdiplus.h>

std::wstring getExeDir();
int checkVillagerProduction(Gdiplus::Bitmap* bmp, const std::string& civilization);
