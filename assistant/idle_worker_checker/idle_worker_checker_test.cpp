#include "idle_worker_checker.h"
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <iostream>
#include <vector>
#include <filesystem>

using namespace Gdiplus;
namespace fs = std::filesystem;

int wmain() {
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) != Ok) {
        std::wcerr << L"Failed to initialize GDI+" << std::endl;
        return 1;
    }

    // Test with the template itself
    std::wstring templatePath = getIdleWorkerCheckerExeDir() + L"\\data\\templates\\no_idle.png";
    std::wcout << L"Testing with template: " << templatePath << std::endl;

    Bitmap* templateBmp = Bitmap::FromFile(templatePath.c_str(), FALSE);
    if (!templateBmp || templateBmp->GetLastStatus() != Ok) {
        std::wcout << L"Failed to load template image" << std::endl;
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    // The template should match itself
    int result = checkIdleWorkers(templateBmp);
    std::wcout << L"Template self-match test: checkIdleWorkers = " << result << L" (expected 1)" << std::endl;
    delete templateBmp;

    // Test with idle status samples (even though they're not designed for this test)
    // This will help verify the function doesn't crash with different inputs
    std::wstring sampleDir = getIdleWorkerCheckerExeDir() + L"\\data\\samples\\idle_status";
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

    // Test with available samples
    for (const auto& file : sampleFiles) {
        Bitmap* bmp = Bitmap::FromFile(file.c_str(), FALSE);
        if (!bmp || bmp->GetLastStatus() != Ok) {
            std::wcout << file << L": Failed to load image" << std::endl;
            delete bmp;
            continue;
        }

        int result = checkIdleWorkers(bmp);
        std::wcout << file << L": checkIdleWorkers = " << result << L" (expected 0)" << std::endl;
        delete bmp;
    }

    GdiplusShutdown(gdiplusToken);

    std::wcout << L"\nPress any key to exit..." << std::endl;
    _getwch();
    return 0;
}
