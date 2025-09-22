#include <ostream>
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include "idle_worker_checker.h"
#include "../../matcher/matcher.h"
#include <iostream>
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <mutex>
#include <experimental/filesystem>
using namespace Gdiplus;
namespace fs = std::experimental::filesystem;

std::wstring getIdleWorkerCheckerExeDir() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    return fs::path(buffer).parent_path();
}

int checkIdleWorkers(Gdiplus::Bitmap* bmp) {
    static Gdiplus::Bitmap* noIdleTemplate = nullptr;
    static std::once_flag template_loaded_flag;
    std::call_once(template_loaded_flag, []() {
        std::wstring templatePath = getIdleWorkerCheckerExeDir() + L"\\data\\templates\\no_idle.png";
        noIdleTemplate = Gdiplus::Bitmap::FromFile(templatePath.c_str(), FALSE);
        if (!noIdleTemplate || noIdleTemplate->GetLastStatus() != Ok) {
            std::wcout << L"Failed to load no_idle template from file:" << templatePath << std::endl;
            delete noIdleTemplate;
            noIdleTemplate = nullptr;
        }
        else {
            std::wcout << L"Successfully loaded no_idle template from file:" << templatePath << std::endl;
        }
    });
    if (!noIdleTemplate) {
        std::cout << "No idle template not found" << std::endl;
        return 0;
    }
    return matchImage(bmp, noIdleTemplate) ? 1 : 0;
}
