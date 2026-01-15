#include "IPCClient.h"
#include <iostream>

namespace Lumos {
    IPCClient::IPCClient() {
    }

    IPCClient::~IPCClient() {
    }

    bool IPCClient::SendPreviewRequest(const PreviewRequest& request) {
        // Ensure UI process is running
        if (!IsUIProcessRunning()) {
            std::wcout << L"UI process not running, attempting to launch..." << std::endl;
            if (!LaunchUIProcess()) {
                std::wcerr << L"Failed to launch UI process" << std::endl;
                return false;
            }
            // Give UI process time to start
            std::wcout << L"Waiting for UI process to initialize..." << std::endl;
            Sleep(1000); // Increased from 500ms to 1000ms
        }

        HANDLE hPipe;
        if (!ConnectToPipe(hPipe)) {
            std::wcerr << L"Failed to connect to named pipe" << std::endl;
            return false;
        }

        // Serialize request to JSON
        std::string json = request.ToJson();
        DWORD bytesWritten;
        
        bool success = WriteFile(
            hPipe,
            json.c_str(),
            static_cast<DWORD>(json.length()),
            &bytesWritten,
            nullptr
        );

        CloseHandle(hPipe);
        
        if (success && bytesWritten == json.length()) {
            std::wcout << L"Successfully sent preview request (" << bytesWritten << L" bytes)" << std::endl;
            return true;
        }
        
        std::wcerr << L"Failed to write to pipe. Bytes written: " << bytesWritten << L" / " << json.length() << std::endl;
        return false;
    }

    bool IPCClient::IsUIProcessRunning() {
        HANDLE hPipe = CreateFile(
            PIPE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
            return true;
        }

        return false;
    }

    bool IPCClient::LaunchUIProcess() {
        std::wstring uiPath = GetUIProcessPath();
        
        std::wcout << L"Attempting to launch UI process: " << uiPath << std::endl;
        
        // Check if file exists
        DWORD fileAttr = GetFileAttributes(uiPath.c_str());
        if (fileAttr == INVALID_FILE_ATTRIBUTES) {
            std::wcerr << L"UI process executable not found at: " << uiPath << std::endl;
            std::wcerr << L"Error code: " << GetLastError() << std::endl;
            return false;
        }
        
        STARTUPINFO si = { sizeof(STARTUPINFO) };
        PROCESS_INFORMATION pi;

        bool success = CreateProcess(
            uiPath.c_str(),
            nullptr,
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi
        );

        if (success) {
            std::wcout << L"UI process launched successfully (PID: " << pi.dwProcessId << L")" << std::endl;
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        } else {
            std::wcerr << L"Failed to create UI process. Error code: " << GetLastError() << std::endl;
        }

        return success;
    }

    bool IPCClient::ConnectToPipe(HANDLE& hPipe) {
        for (int attempt = 0; attempt < 5; ++attempt) {
            hPipe = CreateFile(
                PIPE_NAME,
                GENERIC_READ | GENERIC_WRITE,
                0,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr
            );

            if (hPipe != INVALID_HANDLE_VALUE) {
                return true;
            }

            if (GetLastError() != ERROR_PIPE_BUSY) {
                return false;
            }

            // Wait for pipe to become available
            if (!WaitNamedPipe(PIPE_NAME, PIPE_TIMEOUT_MS)) {
                return false;
            }
        }

        return false;
    }

    std::wstring IPCClient::GetUIProcessPath() {
        wchar_t exePath[MAX_PATH];
        GetModuleFileName(nullptr, exePath, MAX_PATH);
        
        std::wstring path(exePath);
        size_t lastSlash = path.find_last_of(L"\\");
        std::wstring exeDir;
        if (lastSlash != std::wstring::npos) {
            exeDir = path.substr(0, lastSlash + 1);
        }
        
        // Try multiple possible names for the UI executable
        // In MSIX packages, .NET apps may be in win-x64 subdirectory
        std::wstring candidates[] = {
            exeDir + L"ui-managed.exe",                    // Same directory (MSIX - flat)
            exeDir + L"win-x64\\ui-managed.exe",           // MSIX with runtime identifier folder
            exeDir + L"Lumos.UI.exe",                      // Same directory (alternative name)
            exeDir + L"win-x64\\Lumos.UI.exe",             // MSIX RID folder (alternative name)
            exeDir + L"..\\ui-managed\\bin\\x64\\Release\\net8.0-windows\\ui-managed.exe",  // Dev build
            exeDir + L"..\\ui-managed\\bin\\x64\\Release\\net8.0-windows\\Lumos.UI.exe",    // Dev build alternative
            exeDir + L"..\\ui-managed\\bin\\x64\\Debug\\net8.0-windows\\ui-managed.exe",    // Debug build
            exeDir + L"..\\ui-managed\\bin\\x64\\Debug\\net8.0-windows\\Lumos.UI.exe"       // Debug build alternative
        };
        
        for (const auto& candidate : candidates) {
            DWORD fileAttr = GetFileAttributes(candidate.c_str());
            if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
                std::wcout << L"Found UI executable at: " << candidate << std::endl;
                return candidate;
            }
        }
        
        // Default to first candidate if none found
        std::wcout << L"UI executable not found, using default: " << candidates[0] << std::endl;
        return candidates[0];
    }
}
