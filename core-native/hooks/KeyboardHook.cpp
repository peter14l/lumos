#include "KeyboardHook.h"
#include <string>
#include <algorithm>

namespace Lumos {
    KeyboardHook* KeyboardHook::s_instance = nullptr;

    KeyboardHook::KeyboardHook()
        : m_hookHandle(nullptr)
        , m_previewActive(false)
    {
        s_instance = this;
    }

    KeyboardHook::~KeyboardHook() {
        Uninstall();
        s_instance = nullptr;
    }

    bool KeyboardHook::Install() {
        if (m_hookHandle != nullptr) {
            return true; // Already installed
        }

        m_hookHandle = SetWindowsHookEx(
            WH_KEYBOARD_LL,
            LowLevelKeyboardProc,
            GetModuleHandle(nullptr),
            0
        );

        return m_hookHandle != nullptr;
    }

    void KeyboardHook::Uninstall() {
        if (m_hookHandle != nullptr) {
            UnhookWindowsHookEx(m_hookHandle);
            m_hookHandle = nullptr;
        }
    }

    void KeyboardHook::SetSpacebarCallback(SpacebarCallback callback) {
        m_callback = callback;
    }

    LRESULT CALLBACK KeyboardHook::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION && s_instance != nullptr) {
            KBDLLHOOKSTRUCT* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

            // Check for spacebar key down
            if (wParam == WM_KEYDOWN && pKeyboard->vkCode == VK_SPACE) {
                if (s_instance->ShouldTriggerPreview()) {
                    if (s_instance->m_callback) {
                        s_instance->m_callback();
                    }
                }
            }
        }

        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    bool KeyboardHook::ShouldTriggerPreview() {
        // Guard condition 1: Foreground window must be Explorer
        if (!IsForegroundWindowExplorer()) {
            return false;
        }

        // Guard condition 2: No text input focused
        if (IsTextInputFocused()) {
            return false;
        }

        // Guard condition 3: Preview not already active
        if (m_previewActive) {
            return false;
        }

        return true;
    }

    bool KeyboardHook::IsForegroundWindowExplorer() {
        HWND foregroundWindow = GetForegroundWindow();
        if (foregroundWindow == nullptr) {
            return false;
        }

        wchar_t className[256] = { 0 };
        GetClassName(foregroundWindow, className, 256);

        std::wstring classNameStr(className);
        
        // Windows Explorer class names
        return classNameStr == L"CabinetWClass" ||      // File Explorer
               classNameStr == L"ExploreWClass" ||      // Old Explorer
               classNameStr == L"Progman" ||            // Desktop
               classNameStr == L"WorkerW";              // Desktop (worker)
    }

    bool KeyboardHook::IsTextInputFocused() {
        HWND focusedWindow = GetFocus();
        if (focusedWindow == nullptr) {
            return false;
        }

        wchar_t className[256] = { 0 };
        GetClassName(focusedWindow, className, 256);

        std::wstring classNameStr(className);

        // Common text input class names
        return classNameStr == L"Edit" ||
               classNameStr == L"RichEdit" ||
               classNameStr == L"RichEdit20A" ||
               classNameStr == L"RichEdit20W" ||
               classNameStr.find(L"Edit") != std::wstring::npos;
    }
}
