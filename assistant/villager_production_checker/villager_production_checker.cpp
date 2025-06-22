#include <ostream>
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include "villager_production_checker.h"
#include "../../matcher/matcher.h"
#include <iostream>
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <mutex>
#include <experimental/filesystem>
using namespace Gdiplus;
namespace fs = std::experimental::filesystem;


std::wstring getExeDir() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    return fs::path(buffer).parent_path();
}

int checkVillagerProduction(Gdiplus::Bitmap* bmp, const std::string& civilization) {
    static Gdiplus::Bitmap* villagerTemplate = nullptr;
    static std::once_flag template_loaded_flag;
    std::call_once(template_loaded_flag, [&civilization]() {
        std::wstring templatePath = getExeDir() + L"\\data\\templates\\villagers\\" + std::wstring(civilization.begin(), civilization.end()) + L".png";
        villagerTemplate = Gdiplus::Bitmap::FromFile(templatePath.c_str(), FALSE);
        if (!villagerTemplate || villagerTemplate->GetLastStatus() != Ok) {
            std::wcout << L"Failed to load template from file:" << templatePath << std::endl;
            delete villagerTemplate;
            villagerTemplate = nullptr;
        }
        else {
            std::wcout << L"Successfully loaded from file:" << templatePath << std::endl;
        }
    });
    if (!villagerTemplate) {
        std::cout << "Villager template not found" << std::endl;
        return 0;
    }
    return matchImage(bmp, villagerTemplate) ? 1 : 0;
}
