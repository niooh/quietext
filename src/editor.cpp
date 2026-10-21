#include "editor.h"
#include "encoding.h"
#include "fileio.h"
#include <commdlg.h>
#include <cwchar>

namespace editor {

namespace {

HWND         g_main = nullptr;
HWND         g_edit = nullptr;
HFONT        g_font = nullptr;

std::wstring g_path;                 // 空表示未命名
bool         g_dirty = false;
bool         g_suppress = false;     // 抑制 SetWindowTextW 引起的 EN_CHANGE
Enc          g_enc = Enc::UTF8;
bool         g_crlf = true;

std::wstring GetFullPath(const std::wstring& p) {
    wchar_t buf[MAX_PATH * 2];
    DWORD n = GetFullPathNameW(p.c_str(), MAX_PATH * 2, buf, nullptr);
    if (n == 0 || n >= MAX_PATH * 2) return p;
    return std::wstring(buf, n);
}

void CreateFontOnce() {
    if (g_font) return;
    HDC hdc = GetDC(nullptr);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(nullptr, hdc);

    LOGFONTW lf = {};
    lf.lfHeight = -MulDiv(10, dpi, 72);
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    wcsncpy(lf.lfFaceName, L"Consolas", LF_FACESIZE - 1);
    g_font = CreateFontIndirectW(&lf);
    if (!g_font) {
        wcsncpy(lf.lfFaceName, L"Courier New", LF_FACESIZE - 1);
        g_font = CreateFontIndirectW(&lf);
    }
}

void ClearToBlank() {
    g_path.clear();
    g_enc = Enc::UTF8;
    g_crlf = true;
    g_suppress = true;
    SetWindowTextW(g_edit, L"");
    g_suppress = false;
    g_dirty = false;
    ApplyTitle();
}

} // namespace

// ---------- 标题 ----------
void ApplyTitle() {
    if (!g_main) return;
    std::wstring title;
    if (g_path.empty()) title = L"未命名";
    else {
        const wchar_t* p = wcsrchr(g_path.c_str(), L'\\');
        title = p ? (p + 1) : g_path;
    }
    if (g_dirty) title += L" *";
    title += L" - quietext";
    SetWindowTextW(g_main, title.c_str());
}

// ---------- 菜单 ----------
HMENU BuildMenu() {
    HMENU hMenu = CreateMenu();

    HMENU hFile = CreatePopupMenu();
    AppendMenuW(hFile, MF_STRING, ID_FILE_NEW,    L"新建(&N)\tCtrl+N");
    AppendMenuW(hFile, MF_STRING, ID_FILE_OPEN,   L"打开(&O)...\tCtrl+O");
    AppendMenuW(hFile, MF_STRING, ID_FILE_SAVE,   L"保存(&S)\tCtrl+S");
    AppendMenuW(hFile, MF_STRING, ID_FILE_SAVEAS, L"另存为(&A)...\tCtrl+Shift+S");
    AppendMenuW(hFile, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hFile, MF_STRING, ID_FILE_CLOSE,  L"关闭当前(&W)\tCtrl+Q");
    AppendMenuW(hFile, MF_STRING, ID_FILE_EXIT,   L"退出(&X)");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFile, L"文件(&F)");

    HMENU hEdit = CreatePopupMenu();
    AppendMenuW(hEdit, MF_STRING, ID_EDIT_UNDO,      L"撤销(&U)\tCtrl+Z");
    AppendMenuW(hEdit, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hEdit, MF_STRING, ID_EDIT_CUT,       L"剪切(&T)\tCtrl+X");
    AppendMenuW(hEdit, MF_STRING, ID_EDIT_COPY,      L"复制(&C)\tCtrl+C");
    AppendMenuW(hEdit, MF_STRING, ID_EDIT_PASTE,     L"粘贴(&P)\tCtrl+V");
    AppendMenuW(hEdit, MF_STRING, ID_EDIT_SELECTALL, L"全选(&A)\tCtrl+A");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hEdit, L"编辑(&E)");

    HMENU hHelp = CreatePopupMenu();
    AppendMenuW(hHelp, MF_STRING, ID_HELP_ABOUT, L"关于(&A)");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hHelp, L"帮助(&H)");

    return hMenu;
}

// ---------- 绑定 ----------
void Attach(HWND mainWnd, HWND editCtrl) {
    g_main = mainWnd;
    g_edit = editCtrl;
    CreateFontOnce();
    if (g_font) SendMessageW(g_edit, WM_SETFONT, (WPARAM)g_font, TRUE);
    ApplyTitle();
}

// 打开
bool OpenPath(const std::wstring& path) {
    bool existed = GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
    if (!existed) {
        ClearToBlank();
        g_path = GetFullPath(path);
        ApplyTitle();
        return true;
    }

    std::vector<BYTE> data;
    if (!fileio::ReadAllBytes(path, data, g_main)) return false;

    std::wstring text;
    g_enc = DetectAndDecode(data, text);
    g_crlf = IsPureCRLF(text);
    text = ToCRLF(text);

    g_suppress = true;
    SetWindowTextW(g_edit, text.c_str());
    g_suppress = false;

    g_path = GetFullPath(path);
    g_dirty = false;
    ApplyTitle();
    return true;
}

// 保存
bool SaveAs() {
    wchar_t buf[MAX_PATH] = {};
    if (!g_path.empty()) {
        wcsncpy(buf, g_path.c_str(), MAX_PATH - 1);
        buf[MAX_PATH - 1] = 0;
    }
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = g_main;
    ofn.lpstrFilter = L"文本文件\0*.txt\0所有文件\0*.*\0\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile  = MAX_PATH;
    ofn.lpstrDefExt = L"txt";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&ofn)) return false;
    g_path = GetFullPath(buf);
    return true;
}

bool Save() {
    if (g_path.empty()) {
        if (!SaveAs()) return false;
    }
    int len = GetWindowTextLengthW(g_edit);
    std::wstring text(len + 1, L'\0');
    if (len > 0) GetWindowTextW(g_edit, &text[0], len + 1);
    text.resize(len);

    std::wstring out = g_crlf ? text : CRLFtoLF(text);

    bool ok = true;
    std::vector<BYTE> bytes = EncodeText(out, g_enc, ok);
    if (!ok) {
        int r = MessageBoxW(g_main,
            L"当前文件编码无法表示部分字符。\n是否改为 UTF-8（带 BOM）保存？",
            L"quietext", MB_YESNO | MB_ICONQUESTION);
        if (r != IDYES) return false;
        g_enc = Enc::UTF8BOM;
        bytes = EncodeText(out, g_enc, ok);
        if (!ok) {
            MessageBoxW(g_main, L"编码转换失败。", L"quietext", MB_ICONERROR);
            return false;
        }
    }

    if (!fileio::WriteAllBytes(g_path, bytes, g_main)) return false;
    g_dirty = false;
    ApplyTitle();
    return true;
}

bool ConfirmSaveIfDirty() {
    if (!g_dirty) return true;
    int r = MessageBoxW(g_main, L"当前文件已修改，是否保存？",
                        L"quietext", MB_YESNOCANCEL | MB_ICONQUESTION);
    if (r == IDCANCEL) return false;
    if (r == IDYES)    return Save();
    return true;
}

// 对话框
bool DoOpenDialog(std::wstring& out) {
    wchar_t buf[MAX_PATH] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = g_main;
    ofn.lpstrFilter =
        L"文本文件\0*.txt;*.md;*.log;*.ini;*.csv;*.json;*.xml;*.c;*.h;*.cpp;*.hpp;*.py;*.js;*.ts;*.html;*.css;*.sh;*.bat;*.yaml;*.yml;*.toml\0"
        L"所有文件\0*.*\0\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile  = MAX_PATH;
    ofn.Flags     = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&ofn)) return false;
    out = buf;
    return true;
}

// ---------- 文档生命周期 ----------
void NewDocument() {
    if (!ConfirmSaveIfDirty()) return;
    ClearToBlank();
}

void CloseDocument() {
    if (!ConfirmSaveIfDirty()) return;
    ClearToBlank();
}

// ---------- 命令 / 通知 ----------
void OnEditChange() {
    if (g_suppress) return;
    if (!g_dirty) { g_dirty = true; ApplyTitle(); }
}

void OnCommand(UINT id) {
    switch (id) {
    case ID_FILE_NEW:    NewDocument(); break;
    case ID_FILE_OPEN: {
        if (!ConfirmSaveIfDirty()) return;
        std::wstring p;
        if (DoOpenDialog(p)) OpenPath(p);
        break;
    }
    case ID_FILE_SAVE:   Save(); break;
    case ID_FILE_SAVEAS: if (SaveAs()) Save(); break;
    case ID_FILE_CLOSE:  CloseDocument(); break;
    case ID_FILE_EXIT:   SendMessageW(g_main, WM_CLOSE, 0, 0); break;
    case ID_EDIT_UNDO:      SendMessageW(g_edit, WM_UNDO, 0, 0); break;
    case ID_EDIT_CUT:       SendMessageW(g_edit, WM_CUT,   0, 0); break;
    case ID_EDIT_COPY:      SendMessageW(g_edit, WM_COPY,  0, 0); break;
    case ID_EDIT_PASTE:     SendMessageW(g_edit, WM_PASTE, 0, 0); break;
    case ID_EDIT_SELECTALL: SendMessageW(g_edit, EM_SETSEL, 0, -1); break;
    case ID_HELP_ABOUT:
        MessageBoxW(g_main,
            L"quietext\n\n极简文本编辑器。\n保存时不改变文件时间。",
            L"关于 quietext", MB_ICONINFORMATION);
        break;
    }
}

} // namespace editor
