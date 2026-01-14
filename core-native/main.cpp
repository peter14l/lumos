#include <Windows.h>
#include <iostream>
#include "hooks/KeyboardHook.h"
#include "explorer/ExplorerIntegration.h"
#include "ipc/IPCClient.h"

using namespace Lumos;

int main() {
    std::wcout << L"Lumos - Quick Look for Windows" << std::endl;
    std::wcout << L"Initializing..." << std::endl;

    // Initialize Explorer integration
    ExplorerIntegration explorer;
    if (!explorer.Initialize()) {
        std::wcerr << L"Failed to initialize Explorer integration" << std::endl;
        return 1;
    }

    // Create IPC client
    IPCClient ipcClient;

    // Create keyboard hook
    KeyboardHook keyboardHook;

    // Set spacebar callback
    keyboardHook.SetSpacebarCallback([&]() {
        std::wcout << L"Spacebar pressed - checking for selected file..." << std::endl;

        // Get selected file
        auto fileInfo = explorer.GetSelectedFile();
        if (fileInfo.has_value()) {
            std::wcout << L"Selected file: " << fileInfo->path << std::endl;
            std::wcout << L"Extension: " << fileInfo->extension << std::endl;
            std::wcout << L"Size: " << fileInfo->size << L" bytes" << std::endl;

            // Create preview request
            PreviewRequest request;
            request.path = fileInfo->path;
            request.extension = fileInfo->extension;
            request.size = fileInfo->size;

            // Send to UI process
            if (ipcClient.SendPreviewRequest(request)) {
                std::wcout << L"Preview request sent successfully" << std::endl;
            } else {
                std::wcerr << L"Failed to send preview request" << std::endl;
            }
        } else {
            std::wcout << L"No file selected or folder selected" << std::endl;
        }
    });

    // Install keyboard hook
    if (!keyboardHook.Install()) {
        std::wcerr << L"Failed to install keyboard hook" << std::endl;
        return 1;
    }

    std::wcout << L"Lumos is running. Press Spacebar in Explorer to preview files." << std::endl;
    std::wcout << L"Press Ctrl+C to exit." << std::endl;

    // Message loop to keep the application running
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
