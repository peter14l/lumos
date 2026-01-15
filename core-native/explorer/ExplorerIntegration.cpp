#include "ExplorerIntegration.h"
#include <UIAutomation.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <atlbase.h>
#include <algorithm>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Shell32.lib")

namespace Lumos {
    ExplorerIntegration::ExplorerIntegration()
        : m_comInitialized(false)
    {
    }

    ExplorerIntegration::~ExplorerIntegration() {
        if (m_comInitialized) {
            CoUninitialize();
        }
    }

    bool ExplorerIntegration::Initialize() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        m_comInitialized = SUCCEEDED(hr);
        return m_comInitialized;
    }

    std::optional<FileInfo> ExplorerIntegration::GetSelectedFile() {
        FileInfo info;

        // Try UI Automation first (preferred)
        if (GetSelectedFileViaUIAutomation(info)) {
            return info;
        }

        // Fallback to ShellView
        if (GetSelectedFileViaShellView(info)) {
            return info;
        }

        return std::nullopt;
    }

    bool ExplorerIntegration::GetSelectedFileViaUIAutomation(FileInfo& outInfo) {
        // UI Automation is unreliable for getting file paths in Explorer
        // Use ShellView method instead
        return false;
    }

    bool ExplorerIntegration::GetSelectedFileViaShellView(FileInfo& outInfo) {
        // Get the foreground Explorer window
        HWND explorerWindow = GetForegroundWindow();
        if (explorerWindow == nullptr) {
            return false;
        }

        // Find the DirectUIHWND window which contains the file list
        HWND directUIHWND = FindWindowEx(explorerWindow, nullptr, L"ShellTabWindowClass", nullptr);
        if (directUIHWND) {
            directUIHWND = FindWindowEx(directUIHWND, nullptr, L"DUIViewWndClassName", nullptr);
        }
        
        if (!directUIHWND) {
            return false;
        }

        // Try to get IFolderView2 interface
        CComPtr<IShellBrowser> shellBrowser;
        HRESULT hr = IUnknown_QueryService(directUIHWND, SID_STopLevelBrowser, IID_PPV_ARGS(&shellBrowser));
        
        if (FAILED(hr)) {
            // Alternative: try to get from the window itself
            CComPtr<IServiceProvider> serviceProvider;
            hr = IUnknown_QueryService(explorerWindow, SID_STopLevelBrowser, IID_PPV_ARGS(&serviceProvider));
            if (FAILED(hr)) {
                return false;
            }
            
            hr = serviceProvider->QueryService(SID_STopLevelBrowser, IID_PPV_ARGS(&shellBrowser));
            if (FAILED(hr)) {
                return false;
            }
        }

        // Get the active shell view
        CComPtr<IShellView> shellView;
        hr = shellBrowser->QueryActiveShellView(&shellView);
        if (FAILED(hr)) {
            return false;
        }

        // Get IFolderView2 for better file access
        CComPtr<IFolderView2> folderView;
        hr = shellView->QueryInterface(IID_PPV_ARGS(&folderView));
        if (FAILED(hr)) {
            return false;
        }

        // Get selected items
        CComPtr<IShellItemArray> selectedItems;
        hr = folderView->GetSelection(FALSE, &selectedItems);
        if (FAILED(hr)) {
            return false;
        }

        // Get the first selected item
        CComPtr<IShellItem> item;
        hr = selectedItems->GetItemAt(0, &item);
        if (FAILED(hr)) {
            return false;
        }

        // Get the file path
        LPWSTR filePath = nullptr;
        hr = item->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
        if (FAILED(hr) || !filePath) {
            return false;
        }

        std::wstring path(filePath);
        CoTaskMemFree(filePath);

        // Check if it's a directory
        if (IsDirectory(path)) {
            return false;
        }

        outInfo.path = path;
        outInfo.extension = GetFileExtension(path);
        outInfo.size = GetFileSize(path);

        return !outInfo.path.empty();
    }

    bool ExplorerIntegration::IsDirectory(const std::wstring& path) {
        DWORD attributes = GetFileAttributes(path.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) {
            return false;
        }
        return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    std::wstring ExplorerIntegration::GetFileExtension(const std::wstring& path) {
        size_t dotPos = path.find_last_of(L'.');
        if (dotPos != std::wstring::npos && dotPos < path.length() - 1) {
            return path.substr(dotPos);
        }
        return L"";
    }

    uint64_t ExplorerIntegration::GetFileSize(const std::wstring& path) {
        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        if (GetFileAttributesEx(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
            LARGE_INTEGER size;
            size.HighPart = fileInfo.nFileSizeHigh;
            size.LowPart = fileInfo.nFileSizeLow;
            return static_cast<uint64_t>(size.QuadPart);
        }
        return 0;
    }
}
