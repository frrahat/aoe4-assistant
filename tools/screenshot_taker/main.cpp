#include <iostream>
#include <windows.h>
#include "screenshot_taker.h"

int main() {
    std::cout << "Screenshot Taker Tool" << std::endl;
    std::cout << "Press Numpad '*' to take a screenshot" << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;
    std::cout << "Screenshots will be saved to temp/screenshots/" << std::endl << std::endl;
    
    // Initialize GDI+
    if (!initializeGdiPlus()) {
        std::cerr << "Failed to initialize GDI+. Exiting." << std::endl;
        return 1;
    }
    
    // Make sure the process is DPI aware for accurate screenshots
    SetProcessDPIAware();
    
    // Register hotkey (VK_MULTIPLY is the Numpad '*' key)
    // ID 1 is arbitrary, 0 means no modifiers
    if (!RegisterHotKey(NULL, 1, 0, VK_MULTIPLY)) {
        std::cerr << "Failed to register hotkey. Error: " << GetLastError() << std::endl;
        cleanupGdiPlus();
        return 1;
    }
    
    std::cout << "Hotkey registered successfully. Waiting for Numpad '*' key press..." << std::endl;
    
    // Message loop
    MSG msg = {0};
    bool running = true;
    
    while (running) {
        // Check for messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_HOTKEY && msg.wParam == 1) {
                std::cout << "Numpad '*' pressed! Taking screenshot..." << std::endl;
                
                if (takeScreenshot()) {
                    std::cout << "Screenshot taken successfully!" << std::endl;
                } else {
                    std::cout << "Failed to take screenshot!" << std::endl;
                }
                
                std::cout << "Press Numpad '*' for another screenshot, or Ctrl+C to exit." << std::endl;
            }
            else if (msg.message == WM_QUIT) {
                running = false;
            }
        }
        
        // Small sleep to prevent 100% CPU usage
        Sleep(10);
    }
    
    // Cleanup
    UnregisterHotKey(NULL, 1);
    cleanupGdiPlus();
    
    std::cout << "Screenshot tool exited." << std::endl;
    return 0;
}
