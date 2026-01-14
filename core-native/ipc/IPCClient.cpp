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
            if (!LaunchUIProcess()) {
                return false;
            }
            // Give UI process time to start
            Sleep(500);
        }

        HANDLE hPipe;
        if (!ConnectToPipe(hPipe)) {
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
        return success && bytesWritten == json.length();
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
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
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
        if (lastSlash != std::wstring::npos) {
            path = path.substr(0, lastSlash + 1);
        }
        
        path += L"ui-managed.exe";
        return path;
    }
}
