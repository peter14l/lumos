#pragma once
#include <Windows.h>
#include <string>
#include "../shared-contracts/PreviewRequest.h"

namespace Lumos {
    class IPCClient {
    public:
        IPCClient();
        ~IPCClient();

        // Send preview request to UI process
        bool SendPreviewRequest(const PreviewRequest& request);

        // Check if UI process is running
        bool IsUIProcessRunning();

        // Launch UI process if not running
        bool LaunchUIProcess();

    private:
        static constexpr const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\LumosPreview";
        static constexpr DWORD PIPE_TIMEOUT_MS = 1000;

        bool ConnectToPipe(HANDLE& hPipe);
        std::wstring GetUIProcessPath();
    };
}
