#include "ExplorerIntegration.h"
#include <UIAutomation.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <atlbase.h>
#include <algorithm>

#pragma comment(lib, "Shlwapi.lib")

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
        // UI Automation implementation
        CComPtr<IUIAutomation> automation;
        HRESULT hr = CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&automation));
        if (FAILED(hr)) {
            return false;
        }

        // Get foreground window element
        HWND foregroundWindow = GetForegroundWindow();
        CComPtr<IUIAutomationElement> rootElement;
        hr = automation->ElementFromHandle(foregroundWindow, &rootElement);
        if (FAILED(hr)) {
            return false;
        }

        // Find selected item (this is simplified - real implementation would traverse the tree)
        CComPtr<IUIAutomationCondition> condition;
        VARIANT varProp;
        varProp.vt = VT_BOOL;
        varProp.boolVal = VARIANT_TRUE;
        hr = automation->CreatePropertyCondition(UIA_SelectionItemIsSelectedPropertyId, varProp, &condition);
        if (FAILED(hr)) {
            return false;
        }

        CComPtr<IUIAutomationElement> selectedElement;
        hr = rootElement->FindFirst(TreeScope_Descendants, condition, &selectedElement);
        if (FAILED(hr) || selectedElement == nullptr) {
            return false;
        }

        // Get the name property (file path or name)
        CComBSTR name;
        hr = selectedElement->get_CurrentName(&name);
        if (FAILED(hr) || name.Length() == 0) {
            return false;
        }

        std::wstring filePath(name, name.Length());
        
        // Check if it's a directory
        if (IsDirectory(filePath)) {
            return false; // We only want files, not folders
        }

        outInfo.path = filePath;
        outInfo.extension = GetFileExtension(filePath);
        outInfo.size = GetFileSize(filePath);

        return !outInfo.path.empty();
    }

    bool ExplorerIntegration::GetSelectedFileViaShellView(FileInfo& outInfo) {
        // Fallback implementation using IShellView
        HWND explorerWindow = GetForegroundWindow();
        if (explorerWindow == nullptr) {
            return false;
        }

        // Find the ShellView window
        HWND shellView = FindWindowEx(explorerWindow, nullptr, L"SHELLDLL_DefView", nullptr);
        if (shellView == nullptr) {
            return false;
        }

        // Get IShellBrowser
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

        // Get IShellView
        CComPtr<IShellView> shellViewInterface;
        hr = shellBrowser->QueryActiveShellView(&shellViewInterface);
        if (FAILED(hr)) {
            return false;
        }

        // Get selected items
        CComPtr<IDataObject> dataObject;
        hr = shellViewInterface->GetItemObject(SVGIO_SELECTION, IID_PPV_ARGS(&dataObject));
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
