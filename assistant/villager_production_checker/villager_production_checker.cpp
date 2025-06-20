#include "villager_production_checker.h"
#include "../../matcher/matcher.h"
#include <iostream>
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <mutex>
#include <filesystem>
using namespace Gdiplus;
namespace fs = std::filesystem;


std::wstring getExeDir() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    return fs::path(buffer).parent_path();
}

int checkVillagerProduction(Gdiplus::Bitmap* bmp) {
    static Gdiplus::Bitmap* villagerTemplate = nullptr;
    static std::once_flag template_loaded_flag;
    std::call_once(template_loaded_flag, []() {
        std::wstring templatePath = getExeDir() + L"\\data\\templates\\villager_icon.png";
        villagerTemplate = Gdiplus::Bitmap::FromFile(templatePath.c_str(), FALSE);
        if (!villagerTemplate || villagerTemplate->GetLastStatus() != Ok) {
            std::cout << "Failed to load template" << std::endl;
            delete villagerTemplate;
            villagerTemplate = nullptr;
        }
    });
    if (!villagerTemplate) return 0;
    return matchImage(bmp, villagerTemplate) ? 1 : 0;
}
