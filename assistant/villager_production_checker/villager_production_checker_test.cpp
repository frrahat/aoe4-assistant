#include "villager_production_checker.h"
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <iostream>
#include <vector>
#include <filesystem>

using namespace Gdiplus;
namespace fs = std::filesystem;

int wmain() {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) != Ok) {
        std::wcerr << L"Failed to initialize GDI+" << std::endl;
        return 1;
    }
    std::wstring sampleDir = getExeDir() + L"\\data\\samples\\production_queue";
    std::vector<std::wstring> sampleFiles;
    try {
        for (const auto& entry : fs::directory_iterator(sampleDir)) {
            if (entry.is_regular_file()) {
                auto path = entry.path();
                if (path.extension() == L".png") {
                    sampleFiles.push_back(path.wstring());
                }
            }
        }
    } catch (const std::exception& e) {
        std::wcerr << L"Failed to access sample directory: " << sampleDir << L"\nReason: " << e.what() << std::endl;
        GdiplusShutdown(gdiplusToken);
        return 1;
    }
    if (sampleFiles.empty()) {
        std::wcout << L"No PNG samples found in " << sampleDir << std::endl;
    }
    for (const auto& file : sampleFiles) {
        Bitmap* bmp = Bitmap::FromFile(file.c_str(), FALSE);
        if (!bmp || bmp->GetLastStatus() != Ok) {
            std::wcout << file << L": Failed to load image" << std::endl;
            delete bmp;
            continue;
        }
        int result = checkVillagerProduction(bmp);
        std::wcout << file << L": checkVillagerProduction = " << result << std::endl;
        delete bmp;
    }
    GdiplusShutdown(gdiplusToken);
    return 0;
} 