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
        
        if (!m_comInitialized) {
            return false;
        }

        // Initialize UI Automation
        hr = CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_uiAutomation));
        if (FAILED(hr)) {
            return false;
        }

        return true;
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
        if (!m_uiAutomation) {
            return false;
        }

        // Get the foreground window
        HWND foregroundWindow = GetForegroundWindow();
        if (foregroundWindow == nullptr) {
            return false;
        }

        // Create automation element from the window
        CComPtr<IUIAutomationElement> rootElement;
        HRESULT hr = m_uiAutomation->ElementFromHandle(foregroundWindow, &rootElement);
        if (FAILED(hr) || !rootElement) {
            return false;
        }

        // Get the focused element (this should be the selected file item)
        CComPtr<IUIAutomationElement> focusedElement;
        hr = m_uiAutomation->GetFocusedElement(&focusedElement);
        if (FAILED(hr) || !focusedElement) {
            return false;
        }

        // Try to get the file path from the focused element
        std::wstring filePath = GetFilePathFromElement(focusedElement);
        
        if (filePath.empty()) {
            // If focused element doesn't have a path, try to find selected items
            // Create a condition to find selected items
            CComPtr<IUIAutomationCondition> selectedCondition;
            VARIANT varTrue;
            varTrue.vt = VT_BOOL;
            varTrue.boolVal = VARIANT_TRUE;
            hr = m_uiAutomation->CreatePropertyCondition(UIA_SelectionItemIsSelectedPropertyId, varTrue, &selectedCondition);
            
            if (SUCCEEDED(hr) && selectedCondition) {
                CComPtr<IUIAutomationElement> selectedElement;
                hr = rootElement->FindFirst(TreeScope_Descendants, selectedCondition, &selectedElement);
                
                if (SUCCEEDED(hr) && selectedElement) {
                    filePath = GetFilePathFromElement(selectedElement);
                }
            }
        }

        if (!filePath.empty() && !IsDirectory(filePath)) {
            outInfo.path = filePath;
            outInfo.extension = GetFileExtension(filePath);
            outInfo.size = GetFileSize(filePath);
            return true;
        }

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

    std::wstring ExplorerIntegration::GetFilePathFromElement(IUIAutomationElement* element) {
        if (!element) {
            return L"";
        }

        // Strategy 1: Try to get the Value pattern (works for some Explorer views)
        CComPtr<IUIAutomationValuePattern> valuePattern;
        HRESULT hr = element->GetCurrentPatternAs(UIA_ValuePatternId, IID_PPV_ARGS(&valuePattern));
        if (SUCCEEDED(hr) && valuePattern) {
            BSTR value = nullptr;
            hr = valuePattern->get_CurrentValue(&value);
            if (SUCCEEDED(hr) && value) {
                std::wstring result(value);
                SysFreeString(value);
                
                // Check if this is a valid file path
                if (result.length() > 2 && result[1] == L':' && PathFileExists(result.c_str())) {
                    return result;
                }
            }
        }

        // Strategy 2: Try to get the Name property and construct full path
        BSTR name = nullptr;
        hr = element->get_CurrentName(&name);
        if (SUCCEEDED(hr) && name) {
            std::wstring fileName(name);
            SysFreeString(name);

            // Get the current folder path from the Explorer window
            HWND foregroundWindow = GetForegroundWindow();
            if (foregroundWindow) {
                // Try to get the folder path using Shell COM
                CComPtr<IShellWindows> shellWindows;
                hr = CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&shellWindows));
                if (SUCCEEDED(hr)) {
                    long windowCount = 0;
                    shellWindows->get_Count(&windowCount);

                    for (long i = 0; i < windowCount; i++) {
                        CComPtr<IDispatch> dispatch;
                        CComVariant index(i);
                        hr = shellWindows->Item(index, &dispatch);
                        if (FAILED(hr) || !dispatch) continue;

                        CComPtr<IWebBrowserApp> webBrowser;
                        hr = dispatch->QueryInterface(IID_PPV_ARGS(&webBrowser));
                        if (FAILED(hr)) continue;

                        SHANDLE_PTR windowHandle = 0;
                        hr = webBrowser->get_HWND(&windowHandle);
                        if (FAILED(hr) || (HWND)windowHandle != foregroundWindow) continue;

                        // Get the current location URL
                        BSTR locationURL = nullptr;
                        hr = webBrowser->get_LocationURL(&locationURL);
                        if (SUCCEEDED(hr) && locationURL) {
                            std::wstring url(locationURL);
                            SysFreeString(locationURL);

                            // Convert file:/// URL to path
                            if (url.find(L"file:///") == 0) {
                                std::wstring folderPath = url.substr(8); // Remove "file:///"
                                
                                // Replace forward slashes with backslashes
                                std::replace(folderPath.begin(), folderPath.end(), L'/', L'\\');
                                
                                // URL decode
                                wchar_t decodedPath[MAX_PATH];
                                DWORD decodedSize = MAX_PATH;
                                if (SUCCEEDED(PathCreateFromUrl(url.c_str(), decodedPath, &decodedSize, 0))) {
                                    folderPath = decodedPath;
                                }

                                // Construct full file path
                                std::wstring fullPath = folderPath;
                                if (!fullPath.empty() && fullPath.back() != L'\\') {
                                    fullPath += L'\\';
                                }
                                fullPath += fileName;

                                // Verify the file exists
                                if (PathFileExists(fullPath.c_str())) {
                                    return fullPath;
                                }
                            }
                        }
                    }
                }
            }
        }

        return L"";
    }
}
