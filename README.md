<p align="center">
  <img src="docs/figures/logo.png" alt="quietext logo" width="160px"/>
  <br>
  English | <a href="./docs/zh-CN/README.md">简体中文</a>
</p>

A minimal text editor for Windows. It does only one thing: **silently modify text without touching the file timestamp**.

## Features

- **Saving preserves file timestamps**
- **Automatic encoding detection**: UTF-8 (with or without BOM), UTF-16 LE/BE (with BOM), and GBK
- **Line endings**:
  - Files that use pure CRLF remain CRLF after saving; empty files and files with no line breaks are unaffected
  - Pure LF, mixed CRLF/LF, and pure CR files are normalized to LF on save
- Single executable, no runtime dependencies.

## Build

Requirements: Windows 7+, MinGW-w64.

From the project root, run in PowerShell or CMD:

```pwsh
.\build.bat
```

Output is `dist\quietext.exe`, ready to run.

## Install / Uninstall

```pwsh
.\install.bat     # Copy to %LOCALAPPDATA%\quietext\bin and add to the user PATH
.\uninstall.bat   # Reverse the above
```

After installation, reopen your terminal and run `quietext` from any directory.

> When using CMD, the `.\` prefix may be omitted.

## Usage

After installation, `quietext` can be invoked directly from both CMD and PowerShell:

```pwsh
quietext                # Open a blank editor window
quietext .              # If the current directory contains exactly one text file, open it; otherwise show a file picker
quietext new.txt        # Create a new file if it does not exist, or open it if it does
quietext D:\notes\a.md  # Open the specified file
```

If not installed, you can also run the built executable directly:

```pwsh
.\dist\quietext.exe
```

## Shortcuts

| Shortcut | Action |
|---|---|
| Ctrl+N | New blank document |
| Ctrl+O | Open |
| Ctrl+S | Save |
| Ctrl+Shift+S | Save As |
| Ctrl+Q | Close the current document and return to a blank untitled state without exiting the program |
| Ctrl+Z / X / C / V / A | Undo / Cut / Copy / Paste / Select All |

## Known Limitations

- UTF-16 without a BOM cannot be detected automatically, it will be decoded as ACP and show garbled text.
- Saving a read-only file fails, an error dialog will be shown.
