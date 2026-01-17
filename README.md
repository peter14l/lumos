# Lumos – Quick Look for Windows

**Instant file previews in Windows Explorer with a single Spacebar press.**

Lumos brings the beloved macOS Quick Look feature to Windows, allowing you to preview files instantly without opening them in their default applications.

![Lumos Logo](packaging/Assets/Wide310x150Logo.png)

## ✨ Features

- **⌨️ Spacebar Activation**: Press Spacebar in Windows Explorer to instantly preview any selected file
- **🎨 Rich File Support**: Preview images, PDFs, text files, audio, and video
- **⚡ Lightning Fast**: <120ms latency from keypress to preview display
- **🎯 Smart Detection**: Only activates in Windows Explorer, never interferes with other apps
- **🔒 Privacy First**: No telemetry, no network access, completely offline
- **💾 Lightweight**: Minimal memory footprint with intelligent caching

## 📋 Supported File Types

| Category | Extensions |
|----------|-----------|
| **Images** | `.jpg`, `.jpeg`, `.png`, `.gif`, `.bmp`, `.webp`, `.tiff`, `.ico` |
| **Documents** | `.pdf` (requires WebView2 runtime) |
| **Text** | `.txt`, `.md`, `.json`, `.xml`, `.log`, `.cs`, `.cpp`, `.h`, `.py`, `.js`, `.ts`, `.html`, `.css`, `.yaml`, `.yml` |
| **Audio** | `.mp3`, `.wav`, `.flac`, `.m4a`, `.wma`, `.aac`, `.ogg` |
| **Video** | `.mp4`, `.mkv`, `.avi`, `.mov`, `.wmv`, `.flv`, `.webm` |

## 🚀 Installation

### Prerequisites
- Windows 10 (version 1809 or later) or Windows 11
- .NET 8 Runtime (Windows Desktop)
- WebView2 Runtime (for PDF previews)

### Steps

1. **Download** the latest release from the [Releases](https://github.com/yourusername/Lumos/releases) page
2. **Install the certificate** (Required for sideloading):
   - Right-click the downloaded `Lumos_1.0.0.0_x64.msix`
   - Select **Properties** → **Digital Signatures** tab
   - Select the signature in the list and click **Details**
   - Click **View Certificate** → **Install Certificate**
   - Select **Local Machine** → Next
   - Select **Place all certificates in the following store**
   - Browse and select **Trusted Root Certification Authorities** → OK → Next → Finish
   - If prompted for **password**, use **"2008"** and click **OK**
3. **Install Lumos**:
   - Double-click `Lumos_1.0.0.0_x64.msix`
   - Click "Install"
4. **Launch**: Lumos starts automatically and runs in the background

## 📖 Usage

1. Open **Windows Explorer** (File Explorer)
2. Navigate to any folder with files
3. **Select a file** (single click)
4. Press **Spacebar** to preview
5. Press **Spacebar**, **Esc**, or click outside to close the preview

### Tips
- Works on Desktop, in Explorer windows, and in Open/Save dialogs
- Preview window appears near your cursor
- Supports rapid file browsing (just keep pressing Spacebar on different files)
- Does NOT activate when typing in search boxes or renaming files

## ⚠️ Important: Keyboard Hook Disclosure

Lumos uses a **global low-level keyboard hook** to detect Spacebar presses system-wide. This is essential for the Quick Look functionality.

**What this means:**
- Lumos monitors keyboard input to detect when you press Spacebar
- It ONLY acts when Windows Explorer is the active window
- No keystrokes are logged, recorded, or transmitted
- The hook is uninstalled when you close Lumos

**Your privacy:**
- ✅ No data collection
- ✅ No network access
- ✅ Completely offline
- ✅ Open source - verify the code yourself

## 🏗️ Building from Source

### Requirements
- Visual Studio 2022 (with C++ and .NET desktop development workloads)
- Windows SDK 10.0.22621.0 or later
- .NET 8 SDK

### Build Steps

```powershell
# Clone the repository
git clone https://github.com/yourusername/Lumos.git
cd Lumos

# Restore NuGet packages
nuget restore Lumos.sln

# Build the solution
msbuild Lumos.sln /p:Configuration=Release /p:Platform=x64

# Or open Lumos.sln in Visual Studio and build
```

### Project Structure

```
Lumos/
├── core-native/        # C++ keyboard hook and Explorer integration
├── ui-managed/         # C# WPF preview window and renderers
├── shared-contracts/   # IPC data structures
├── packaging/          # MSIX packaging project
└── .github/workflows/  # CI/CD automation
```

## 🔧 Troubleshooting

### Preview doesn't appear when pressing Spacebar
- Ensure Lumos is running (check system tray)
- Verify you're in Windows Explorer (not another app)
- Make sure a file (not folder) is selected
- Try restarting Lumos

### PDF previews don't work
- Install [WebView2 Runtime](https://developer.microsoft.com/en-us/microsoft-edge/webview2/)
- Restart Lumos after installing

### Installation fails
- Ensure the certificate is installed in "Trusted Root Certification Authorities"
- Run PowerShell as Administrator and execute:
  ```powershell
  Add-AppxPackage -Path "Lumos_1.0.0.0_x64.msix"
  ```

### High memory usage
- Lumos caches the last 5 previews in RAM
- Close and reopen Lumos to clear the cache
- Large video files may temporarily use more memory

## 🗑️ Uninstallation

1. Open **Settings** → **Apps** → **Installed apps**
2. Find **Lumos** in the list
3. Click the three dots → **Uninstall**
4. Confirm uninstallation

Lumos will be completely removed with no background services remaining.

## 🛡️ Security & Privacy

- **No telemetry**: Lumos does not collect any usage data
- **No network access**: Completely offline, no internet connection required
- **No file modifications**: Read-only access to files
- **No registry pollution**: Minimal system footprint
- **Open source**: Full transparency - inspect the code yourself

## 📜 License

MIT License - see [LICENSE](LICENSE) file for details

## 🙏 Acknowledgments

- Inspired by macOS Quick Look
- Built with love for the Windows community
- Special thanks to all contributors

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/yourusername/Lumos/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/Lumos/discussions)

---

**Made with ❤️ for Windows users who miss Quick Look**
