#include "ExplorerIntegration.h"
#include <UIAutomation.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <exdisp.h>
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
        HWND foregroundWindow = GetForegroundWindow();
        if (foregroundWindow == nullptr) {
            return false;
        }

        // Use Shell Windows to enumerate Explorer windows and find the active one
        CComPtr<IShellWindows> shellWindows;
        HRESULT hr = CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&shellWindows));
        if (FAILED(hr)) {
            return false;
        }

        long windowCount = 0;
        hr = shellWindows->get_Count(&windowCount);
        if (FAILED(hr)) {
            return false;
        }

        // Find the Explorer window that matches our foreground window
        for (long i = 0; i < windowCount; i++) {
            CComPtr<IDispatch> dispatch;
            CComVariant index(i);
            hr = shellWindows->Item(index, &dispatch);
            if (FAILED(hr) || !dispatch) {
                continue;
            }

            // Get IWebBrowserApp interface
            CComPtr<IWebBrowserApp> webBrowser;
            hr = dispatch->QueryInterface(IID_PPV_ARGS(&webBrowser));
            if (FAILED(hr)) {
                continue;
            }

            // Get the window handle
            SHANDLE_PTR windowHandle = 0;
            hr = webBrowser->get_HWND(&windowHandle);
            if (FAILED(hr)) {
                continue;
            }

            // Check if this is our foreground window
            if ((HWND)windowHandle != foregroundWindow) {
                continue;
            }

            // Get IServiceProvider
            CComPtr<IServiceProvider> serviceProvider;
            hr = webBrowser->QueryInterface(IID_PPV_ARGS(&serviceProvider));
            if (FAILED(hr)) {
                continue;
            }

            // Get IShellBrowser
            CComPtr<IShellBrowser> shellBrowser;
            hr = serviceProvider->QueryService(SID_STopLevelBrowser, IID_PPV_ARGS(&shellBrowser));
            if (FAILED(hr)) {
                continue;
            }

            // Get IShellView
            CComPtr<IShellView> shellView;
            hr = shellBrowser->QueryActiveShellView(&shellView);
            if (FAILED(hr)) {
                continue;
            }

            // Get selected items using IDataObject
            CComPtr<IDataObject> dataObject;
            hr = shellView->GetItemObject(SVGIO_SELECTION, IID_PPV_ARGS(&dataObject));
            if (FAILED(hr)) {
                continue;
            }

            // Get file path from data object
            FORMATETC format = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            STGMEDIUM medium;
            hr = dataObject->GetData(&format, &medium);
            if (FAILED(hr)) {
                continue;
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

        return false;
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
