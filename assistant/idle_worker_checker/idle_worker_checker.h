#pragma once
#include <string>
#include <windows.h>
#include <gdiplus.h>

std::wstring getIdleWorkerCheckerExeDir();
int checkIdleWorkers(Gdiplus::Bitmap* bmp);
