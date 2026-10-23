#include "editor.h"
#include "encoding.h"
#include "fileio.h"

#include <commdlg.h>
#include <cwchar>

namespace editor {

namespace {

HWND g_main = nullptr;
HWND g_edit = nullptr;
HFONT g_font = nullptr;

std::wstring g_path;
bool g_dirty = false;
bool g_suppress = false;

Enc g_enc = Enc::UTF8;
bool g_crlf = true;

void ShowError(const wchar_t* message) {
    MessageBoxW(g_main, message, L"quietext", MB_ICONERROR);
}

// Replace edit text without marking the document dirty.
void SetEditText(const std::wstring& text) {
    g_suppress = true;
    SetWindowTextW(g_edit, text.c_str());
    g_suppress = false;
}

void CreateFontOnce() {
    if (g_font) return;

    HDC dc = GetDC(nullptr);
    int dpi = GetDeviceCaps(dc, LOGPIXELSY);
    ReleaseDC(nullptr, dc);

    LOGFONTW font{};
    font.lfHeight = -MulDiv(10, dpi, 72);
    font.lfWeight = FW_NORMAL;
    font.lfCharSet = DEFAULT_CHARSET;
    wcsncpy(font.lfFaceName, L"Consolas", LF_FACESIZE - 1);
    font.lfFaceName[LF_FACESIZE - 1] = 0;

    g_font = CreateFontIndirectW(&font);
    if (!g_font) {
        // Fallback for systems without Consolas.
        wcsncpy(font.lfFaceName, L"Courier New", LF_FACESIZE - 1);
        font.lfFaceName[LF_FACESIZE - 1] = 0;
        g_font = CreateFontIndirectW(&font);
    }
}

void ClearToBlank() {
    g_path.clear();
    g_enc = Enc::UTF8;
    g_crlf = true;
    SetEditText(L"");
    g_dirty = false;
    ApplyTitle();
}

bool PickSavePath(std::wstring& path) {
    wchar_t buffer[MAX_PATH]{};
    if (!g_path.empty()) {
        wcsncpy(buffer, g_path.c_str(), MAX_PATH - 1);
        buffer[MAX_PATH - 1] = 0;
    }

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner   = g_main;
    dialog.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0\0";
    dialog.lpstrFile   = buffer;
    dialog.nMaxFile    = MAX_PATH;
    dialog.lpstrTitle  = L"Save As";
    dialog.lpstrDefExt = L"txt";
    dialog.Flags       = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (!GetSaveFileNameW(&dialog)) return false;

    path = fileio::GetFullPath(buffer);
    return true;
}

bool SaveToPath(const std::wstring& path) {
    int length = GetWindowTextLengthW(g_edit);
    if (length < 0) return false;

    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    if (length > 0) {
        int copied = GetWindowTextW(g_edit, &text[0], length + 1);
        if (copied != length) { ShowError(L"Cannot read editor contents."); return false; }
    }
    text.resize(static_cast<size_t>(length));

    std::wstring output = g_crlf ? text : CRLFtoLF(text);
    Enc saveEncoding = g_enc;
    bool ok = true;
    std::vector<BYTE> bytes = EncodeText(output, saveEncoding, ok);

    if (!ok) {
        int result = MessageBoxW(g_main,
            L"Some characters cannot be represented in the current encoding.\n"
            L"Save as UTF-8 with BOM instead?",
            L"quietext", MB_YESNO | MB_ICONQUESTION);
        if (result != IDYES) return false;

        saveEncoding = Enc::UTF8BOM;
        bytes = EncodeText(output, saveEncoding, ok);
        if (!ok) { ShowError(L"Encoding conversion failed."); return false; }
    }

    if (!fileio::WriteAllBytes(path, bytes, g_main)) return false;

    g_enc = saveEncoding;
    g_dirty = false;
    ApplyTitle();
    return true;
}

} // namespace

void ApplyTitle() {
    if (!g_main) return;

    std::wstring title;
    if (g_path.empty()) {
        title = L"Untitled";
    } else {
        const wchar_t* name = wcsrchr(g_path.c_str(), L'\\');
        title = name ? name + 1 : g_path;
    }
    if (g_dirty) title += L" *";
    title += L" - quietext";

    SetWindowTextW(g_main, title.c_str());
}

HMENU BuildMenu() {
    HMENU menu = CreateMenu();

    HMENU fileMenu = CreatePopupMenu();
    AppendMenuW(fileMenu, MF_STRING, ID_FILE_NEW,    L"&New\tCtrl+N");
    AppendMenuW(fileMenu, MF_STRING, ID_FILE_OPEN,   L"&Open...\tCtrl+O");
    AppendMenuW(fileMenu, MF_STRING, ID_FILE_SAVE,   L"&Save\tCtrl+S");
    AppendMenuW(fileMenu, MF_STRING, ID_FILE_SAVEAS, L"Save &As...\tCtrl+Shift+S");
    AppendMenuW(fileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(fileMenu, MF_STRING, ID_FILE_CLOSE,  L"&Close\tCtrl+Q");
    AppendMenuW(fileMenu, MF_STRING, ID_FILE_EXIT,   L"E&xit");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(fileMenu), L"&File");

    HMENU editMenu = CreatePopupMenu();
    AppendMenuW(editMenu, MF_STRING, ID_EDIT_UNDO,      L"&Undo\tCtrl+Z");
    AppendMenuW(editMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(editMenu, MF_STRING, ID_EDIT_CUT,       L"Cu&t\tCtrl+X");
    AppendMenuW(editMenu, MF_STRING, ID_EDIT_COPY,      L"&Copy\tCtrl+C");
    AppendMenuW(editMenu, MF_STRING, ID_EDIT_PASTE,     L"&Paste\tCtrl+V");
    AppendMenuW(editMenu, MF_STRING, ID_EDIT_SELECTALL, L"Select &All\tCtrl+A");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(editMenu), L"&Edit");

    HMENU helpMenu = CreatePopupMenu();
    AppendMenuW(helpMenu, MF_STRING, ID_HELP_ABOUT, L"&About");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(helpMenu), L"&Help");

    return menu;
}

void Attach(HWND mainWnd, HWND editCtrl) {
    g_main = mainWnd;
    g_edit = editCtrl;

    CreateFontOnce();
    if (g_font) {
        SendMessageW(g_edit, WM_SETFONT,
                     reinterpret_cast<WPARAM>(g_font), TRUE);
    }
    ApplyTitle();
}

bool OpenPath(const std::wstring& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());

    if (attributes == INVALID_FILE_ATTRIBUTES) {
        DWORD error = GetLastError();
        // Missing file: treat as a new document at that path (matches editor UX).
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
            ClearToBlank();
            g_path = fileio::GetFullPath(path);
            ApplyTitle();
            return true;
        }
        ShowError(L"Cannot access file.");
        return false;
    }

    if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
        ShowError(L"The specified path is a directory.");
        return false;
    }

    std::vector<BYTE> data;
    if (!fileio::ReadAllBytes(path, data, g_main)) return false;

    std::wstring text;
    Enc encoding;
    if (!DetectAndDecode(data, text, encoding)) {
        ShowError(L"The file could not be decoded safely.");
        return false;
    }
    if (text.find(L'\0') != std::wstring::npos) {
        ShowError(L"This file contains NUL characters and cannot be safely edited.");
        return false;
    }

    // Remember the original newline style, but edit internally as CRLF.
    bool crlf = IsPureCRLF(text);
    text = ToCRLF(text);

    SetEditText(text);

    g_path = fileio::GetFullPath(path);
    g_enc = encoding;
    g_crlf = crlf;
    g_dirty = false;
    ApplyTitle();
    return true;
}

bool Save() {
    if (g_path.empty()) return SaveAs();
    return SaveToPath(g_path);
}

bool SaveAs() {
    std::wstring newPath;
    if (!PickSavePath(newPath)) return false;
    if (!SaveToPath(newPath)) return false;

    g_path = newPath;
    ApplyTitle();
    return true;
}

bool ConfirmSaveIfDirty() {
    if (!g_dirty) return true;

    int result = MessageBoxW(g_main,
        L"The document has been modified. Save changes?",
        L"quietext", MB_YESNOCANCEL | MB_ICONQUESTION);

    if (result == IDCANCEL) return false;
    if (result == IDYES)    return Save();
    return true;
}

bool DoOpenDialog(std::wstring& out) {
    wchar_t buffer[MAX_PATH]{};

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner   = g_main;
    dialog.lpstrFilter = L"All Files\0*.*\0\0";
    dialog.lpstrFile   = buffer;
    dialog.nMaxFile    = MAX_PATH;
    dialog.lpstrTitle  = L"Open";
    dialog.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&dialog)) return false;

    out = buffer;
    return true;
}

void NewDocument() {
    if (!ConfirmSaveIfDirty()) return;
    ClearToBlank();
}

void CloseDocument() {
    if (!ConfirmSaveIfDirty()) return;
    ClearToBlank();
}

void OnEditChange() {
    if (g_suppress || g_dirty) return;
    g_dirty = true;
    ApplyTitle();
}

void OnCommand(UINT id) {
    switch (id) {
    case ID_FILE_NEW:  NewDocument(); break;
    case ID_FILE_OPEN: {
        if (!ConfirmSaveIfDirty()) return;
        std::wstring path;
        if (DoOpenDialog(path)) OpenPath(path);
        break;
    }
    case ID_FILE_SAVE:   Save();     break;
    case ID_FILE_SAVEAS: SaveAs();   break;
    case ID_FILE_CLOSE:  CloseDocument(); break;

    case ID_FILE_EXIT:
        SendMessageW(g_main, WM_CLOSE, 0, 0);
        break;

    case ID_EDIT_UNDO:      SendMessageW(g_edit, WM_UNDO, 0, 0);   break;
    case ID_EDIT_CUT:       SendMessageW(g_edit, WM_CUT,  0, 0);   break;
    case ID_EDIT_COPY:      SendMessageW(g_edit, WM_COPY, 0, 0);   break;
    case ID_EDIT_PASTE:     SendMessageW(g_edit, WM_PASTE,0, 0);   break;
    case ID_EDIT_SELECTALL: SendMessageW(g_edit, EM_SETSEL, 0, -1); break;

    case ID_HELP_ABOUT:
        MessageBoxW(g_main,
            L"quietext\n\n"
            L"A minimal timestamp-preserving text editor.\n\n"
            L"Existing files are modified in place while preserving "
            L"CreationTime, LastAccessTime and LastWriteTime whenever possible.",
            L"About quietext", MB_ICONINFORMATION);
        break;
    }
}

} // namespace editor
