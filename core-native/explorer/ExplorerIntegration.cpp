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

        // Find the ShellView window - try multiple window class names
        HWND shellViewWindow = FindWindowEx(explorerWindow, nullptr, L"SHELLDLL_DefView", nullptr);
        if (!shellViewWindow) {
            // Try alternative window hierarchy for Windows 11
            HWND shellTabWindow = FindWindowEx(explorerWindow, nullptr, L"ShellTabWindowClass", nullptr);
            if (shellTabWindow) {
                shellViewWindow = FindWindowEx(shellTabWindow, nullptr, L"SHELLDLL_DefView", nullptr);
            }
        }
        
        if (!shellViewWindow) {
            return false;
        }

        // Get IShellBrowser from the window
        CComPtr<IUnknown> unknown;
        HRESULT hr = SHGetInstanceExplorer(&unknown);
        if (FAILED(hr)) {
            return false;
        }
        
        CComPtr<IShellBrowser> shellBrowser;
        hr = unknown->QueryInterface(IID_PPV_ARGS(&shellBrowser));
        if (FAILED(hr)) {
            return false;
        }

        // Get the active shell view
        CComPtr<IShellView> shellView;
        hr = shellBrowser->QueryActiveShellView(&shellView);
        if (FAILED(hr)) {
            return false;
        }

        // Try to get IFolderView2 for better file access
        CComPtr<IFolderView2> folderView;
        hr = shellView->QueryInterface(IID_PPV_ARGS(&folderView));
        if (SUCCEEDED(hr) && folderView) {
            // Use IFolderView2 to get selected items
            CComPtr<IShellItemArray> selectedItems;
            hr = folderView->GetSelection(FALSE, &selectedItems);
            if (SUCCEEDED(hr) && selectedItems) {
                // Get the first selected item
                CComPtr<IShellItem> item;
                hr = selectedItems->GetItemAt(0, &item);
                if (SUCCEEDED(hr) && item) {
                    // Get the file path
                    LPWSTR filePath = nullptr;
                    hr = item->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
                    if (SUCCEEDED(hr) && filePath) {
                        std::wstring path(filePath);
                        CoTaskMemFree(filePath);

                        // Check if it's a directory
                        if (!IsDirectory(path)) {
                            outInfo.path = path;
                            outInfo.extension = GetFileExtension(path);
                            outInfo.size = GetFileSize(path);
                            return true;
                        }
                    }
                }
            }
        }

        // Fallback: use IDataObject method
        CComPtr<IDataObject> dataObject;
        hr = shellView->GetItemObject(SVGIO_SELECTION, IID_PPV_ARGS(&dataObject));
        if (FAILED(hr)) {
            return false;
        }

        // Get file path from data object
        FORMATETC format = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM medium;
        hr = dataObject->GetData(&format, &medium);
        if (FAILED(hr)) {
            return false;
        }

        HDROP hDrop = static_cast<HDROP>(GlobalLock(medium.hGlobal));
        if (hDrop != nullptr) {
            UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
            if (fileCount > 0) {
                wchar_t filePath[MAX_PATH];
                DragQueryFile(hDrop, 0, filePath, MAX_PATH);
                
                if (!IsDirectory(filePath)) {
                    outInfo.path = filePath;
                    outInfo.extension = GetFileExtension(filePath);
                    outInfo.size = GetFileSize(filePath);
                }
            }
            GlobalUnlock(medium.hGlobal);
        }

        ReleaseStgMedium(&medium);

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
