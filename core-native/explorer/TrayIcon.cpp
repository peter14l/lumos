#include "TrayIcon.h"
#include <iostream>

#define ID_TRAY_APP_ICON 1001
#define ID_TRAY_EXIT 1002
#define ID_TRAY_RESTORE 1003
#define IDT_CONSOLE_CHECK_TIMER 1

namespace Lumos {

    TrayIcon* TrayIcon::s_instance = nullptr;

    TrayIcon::TrayIcon() : m_hHiddenWindow(nullptr) {
        s_instance = this;
        ZeroMemory(&m_nid, sizeof(m_nid));
    }

    TrayIcon::~TrayIcon() {
        Cleanup();
    }

    bool TrayIcon::Initialize(HINSTANCE hInstance) {
        // Register hidden window class
        WNDCLASSEX wc = { 0 };
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = L"LumosTrayIconHiddenWindow";

        if (!RegisterClassEx(&wc)) {
            return false;
        }

        // Create hidden window (message-only)
        m_hHiddenWindow = CreateWindowEx(0, L"LumosTrayIconHiddenWindow", L"Lumos Tray Helper", 
                                       0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

        if (!m_hHiddenWindow) {
            return false;
        }

        // Initialize NotifyIconData
        m_nid.cbSize = sizeof(NOTIFYICONDATA);
        m_nid.hWnd = m_hHiddenWindow;
        m_nid.uID = ID_TRAY_APP_ICON;
        m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        m_nid.uCallbackMessage = WM_TRAYICON;
        
        // Use standard application icon (or generic one if fails)
        m_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION); 
        wcscpy_s(m_nid.szTip, L"Lumos");

        if (!Shell_NotifyIcon(NIM_ADD, &m_nid)) {
            std::wcerr << L"Failed to add tray icon" << std::endl;
            return false;
        }

        // Start timer to check for console minimization (every 500ms)
        SetTimer(m_hHiddenWindow, IDT_CONSOLE_CHECK_TIMER, 500, NULL);

        return true;
    }

    void TrayIcon::Cleanup() {
        if (m_hHiddenWindow) {
            Shell_NotifyIcon(NIM_DELETE, &m_nid);
            DestroyWindow(m_hHiddenWindow);
            m_hHiddenWindow = nullptr;
        }
    }

    LRESULT CALLBACK TrayIcon::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (s_instance) {
            switch (msg) {
                case WM_TRAYICON:
                    if (lParam == WM_LBUTTONDBLCLK) {
                        s_instance->RestoreConsole();
                    } else if (lParam == WM_RBUTTONUP) {
                        s_instance->InitializeMenu(hwnd);
                    }
                    break;

                case WM_COMMAND:
                    switch (LOWORD(wParam)) {
                        case ID_TRAY_RESTORE:
                            s_instance->RestoreConsole();
                            break;
                        case ID_TRAY_EXIT:
                            PostQuitMessage(0); // This will break the main loop in main.cpp
                            break;
                    }
                    break;

                case WM_TIMER:
                    if (wParam == IDT_CONSOLE_CHECK_TIMER) {
                        s_instance->CheckConsoleState();
                    }
                    break;
            }
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void TrayIcon::InitializeMenu(HWND hwnd) {
        POINT pt;
        GetCursorPos(&pt);
        HMENU hMenu = CreatePopupMenu();
        if (hMenu) {
            InsertMenu(hMenu, -1, MF_BYPOSITION, ID_TRAY_RESTORE, L"Show Console");
            InsertMenu(hMenu, -1, MF_BYPOSITION, ID_TRAY_EXIT, L"Quit Lumos");

            SetForegroundWindow(hwnd); // Required for menu to close properly
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
    }

    void TrayIcon::CheckConsoleState() {
        HWND hConsole = GetConsoleWindow();
        if (hConsole && IsIconic(hConsole)) {
            // Console is minimized, hide it completely
            ShowWindow(hConsole, SW_HIDE);
        }
    }

    void TrayIcon::HideConsole() {
        HWND hConsole = GetConsoleWindow();
        if (hConsole) ShowWindow(hConsole, SW_HIDE);
    }

    void TrayIcon::RestoreConsole() {
        HWND hConsole = GetConsoleWindow();
        if (hConsole) {
            ShowWindow(hConsole, SW_RESTORE);
            SetForegroundWindow(hConsole);
        }
    }
}
