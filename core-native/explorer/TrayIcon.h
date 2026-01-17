#pragma once
#include <Windows.h>
#include <shellapi.h>
#include <string>
#include <functional>

#define WM_TRAYICON (WM_USER + 1)

namespace Lumos {
    class TrayIcon {
    public:
        TrayIcon();
        ~TrayIcon();

        bool Initialize(HINSTANCE hInstance);
        void Cleanup();

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        void InitializeMenu(HWND hwnd);
        void CheckConsoleState();
        void RestoreConsole();
        void HideConsole();

        HWND m_hHiddenWindow;
        NOTIFYICONDATA m_nid;
        static TrayIcon* s_instance;
    };
}
