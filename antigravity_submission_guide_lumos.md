# Lumos – Antigravity Build & Packaging Guide

**Target Output:** Signed **MSIX package**, ready for public distribution  
**Platform:** Windows 10 / 11 (x64)  
**CI/CD:** GitHub Actions  
**Build System:** Hybrid **C++ (core)** + **C# WPF (.NET)**  
**Code Generation:** Antigravity

---

## 1. Project Overview

**Lumos** is a macOS-style *Quick Look* utility for Windows.

### Core Behavior
- User selects a file in Windows Explorer
- Presses **Spacebar**
- An instant, lightweight preview appears
- Press **Space / Esc / click outside** to dismiss

### Design Principles
- Zero UI friction
- Extremely low latency (<120ms target)
- Read-only, non-destructive
- No telemetry, no network access

---

## 2. Repository Structure (Must Be Preserved)

```
Lumos/
├── core-native/            # C++ low-level core
│   ├── hooks/
│   ├── explorer/
│   ├── ipc/
│   └── core-native.vcxproj
│
├── ui-managed/             # C# WPF UI
│   ├── App.xaml
│   ├── PreviewWindow.xaml
│   ├── Renderers/
│   └── ui-managed.csproj
│
├── shared-contracts/       # IPC models
│   ├── PreviewRequest.cs
│   └── PreviewRequest.h
│
├── packaging/
│   ├── Package.appxmanifest
│   ├── Assets/
│   └── Lumos.wapproj
│
├── .github/workflows/
│   └── build-msix.yml
│
└── README.md
```

Antigravity **must not flatten or rename** this structure.

---

### Window Composition & Backdrop Strategy

The application uses a single top-level WPF window with a system-backed
Mica backdrop applied at the HWND level via DWM APIs.

All UI panes and controls are layered above a fully transparent root container.
No pane is allowed to use opaque system theme brushes that would occlude
the backdrop.

Mica is treated as a window-level material, not a control background.

---

## 3. Technology Choices (Non-Negotiable)

### C++ Core (core-native)
- Language: **C++17**
- Compiler: **MSVC**
- Runtime: Static where possible

**Responsibilities**
- Global keyboard hook (Spacebar)
- Explorer focus detection
- Selected file path extraction
- IPC message dispatch

### C# UI (ui-managed)
- Framework: **.NET 8 (WPF)**
- Rendering only
- No global hooks

**Responsibilities**
- Preview window lifecycle
- File-type-specific renderers
- Animations and layout

### IPC
- Mechanism: **Named Pipes**
- One-way request from core → UI
- JSON payloads

---

## 4. Core-Native Requirements (C++)

### Keyboard Hook
- Use `SetWindowsHookEx(WH_KEYBOARD_LL)`
- Trigger only on **Spacebar key-down**

### Guard Conditions (ALL REQUIRED)
- Foreground window must be Explorer
- No focused text input
- A file (not folder) is selected
- UI preview window not already active

### Explorer Detection
- Preferred: UI Automation API
- Fallback: IShellView / COM

### Output Payload
```json
{
  "path": "C:\\Users\\...\\file.pdf",
  "extension": ".pdf",
  "size": 2048576
}
```

---

## 5. UI-Managed Requirements (C# WPF)

### Preview Window Rules
- Borderless
- No taskbar icon
- Always-on-top
- Auto-close on focus loss

### Layout Constraints
- Max width/height: 80% of screen
- Centered near cursor position
- Subtle fade-in (<100ms)

### Renderer Matrix

| File Type | Implementation |
|--------|----------------|
| Images | BitmapImage |
| PDF | WebView2 or PDFium |
| Text | Read-only TextBox |
| Audio | MediaElement |
| Video | MediaElement |

Unsupported formats must show a graceful message.

---

## 6. Performance Rules (Strict)

- Lazy-load large files
- Abort rendering immediately on close
- Cap image resolution
- Cache last 5 previews (RAM only)

**Hard rule:** No preview operation may block the UI thread.

---

## 7. Security & Permissions

- No file writes
- No registry writes (except startup toggle later)
- No admin privileges
- No network access

Keyboard hook usage **must be documented clearly**.

---

## 8. MSIX Packaging Requirements

### Packaging Project
- Use **Windows Application Packaging Project (.wapproj)**
- Output format: **MSIX**

### App Identity
- Name: `Lumos`
- Publisher: `CN=Lumos`
- Version: `1.0.0.0`

### Capabilities
- `runFullTrust`

No other capabilities allowed.

---

## 9. Code Signing (MANDATORY)

### Certificate
- Self-signed certificate generated in CI
- Used to sign MSIX

### Signing Steps
1. Generate certificate
2. Install to local machine store
3. Sign MSIX during build

Unsigned packages are **NOT ACCEPTABLE**.

---

## 10. GitHub Actions Workflow Expectations

### Required Steps
1. Checkout repository
2. Setup MSVC build tools
3. Setup .NET SDK
4. Build C++ core
5. Build C# UI
6. Build MSIX package
7. Sign MSIX
8. Upload artifact

### Artifact Output
- `Lumos_1.0.0.0_x64.msix`

---

## 11. Testing Expectations

Antigravity-generated code must pass:

- Spacebar press in Explorer triggers preview
- Spacebar press in other apps does nothing
- Preview closes instantly on Esc
- Rapid open/close does not crash
- Large files do not freeze UI

---

## 12. Explicit Non-Goals (Do NOT Implement)

- Editing files
- Cloud sync
- Telemetry
- Analytics
- Auto-updaters
- Feature overlap with PowerToys

---

## 13. Final Output Checklist

Before submission, ensure:
- [ ] MSIX is signed
- [ ] Installs without warnings
- [ ] Uninstalls cleanly
- [ ] No background services remain
- [ ] README explains keyboard hook usage

---

## 14. Acceptance Criteria

This build is considered **successful** only if:
- MSIX installs on a clean Windows machine
- Spacebar Quick Look works reliably
- No PowerToys feature overlap exists
- Performance feels instant

---

**This document is authoritative.**  
Antigravity should generate and modify code **only within these constraints**.

