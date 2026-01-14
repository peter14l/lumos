#pragma once
#include <Windows.h>
#include <functional>

namespace Lumos {
    class KeyboardHook {
    public:
        using SpacebarCallback = std::function<void()>;

        KeyboardHook();
        ~KeyboardHook();

        // Install the keyboard hook
        bool Install();

        // Uninstall the keyboard hook
        void Uninstall();

        // Set callback for spacebar press
        void SetSpacebarCallback(SpacebarCallback callback);

        // Check if hook is installed
        bool IsInstalled() const { return m_hookHandle != nullptr; }

    private:
        static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
        static KeyboardHook* s_instance;

        bool ShouldTriggerPreview();
        bool IsForegroundWindowExplorer();
        bool IsTextInputFocused();

        HHOOK m_hookHandle;
        SpacebarCallback m_callback;
        bool m_previewActive;
    };
}
