#pragma once
#include <Windows.h>
#include <string>
#include <optional>
#include <UIAutomation.h>
#include <atlbase.h>

namespace Lumos {
    struct FileInfo {
        std::wstring path;
        std::wstring extension;
        uint64_t size;
    };

    class ExplorerIntegration {
    public:
        ExplorerIntegration();
        ~ExplorerIntegration();

        // Initialize COM and UI Automation
        bool Initialize();

        // Get currently selected file in Explorer
        std::optional<FileInfo> GetSelectedFile();

    private:
        bool GetSelectedFileViaUIAutomation(FileInfo& outInfo);
        bool GetSelectedFileViaShellView(FileInfo& outInfo);
        bool IsDirectory(const std::wstring& path);
        std::wstring GetFileExtension(const std::wstring& path);
        uint64_t GetFileSize(const std::wstring& path);
        std::wstring GetFilePathFromElement(IUIAutomationElement* element);

        bool m_comInitialized;
        CComPtr<IUIAutomation> m_uiAutomation;
    };
}
