#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <cwctype>
#include "editor.h"

static HWND g_hMain = nullptr;
static HWND g_hEdit = nullptr;

static std::wstring GetFullPath(const std::wstring& p) {
    wchar_t buf[MAX_PATH * 2];
    DWORD n = GetFullPathNameW(p.c_str(), MAX_PATH * 2, buf, nullptr);
    if (n == 0 || n >= MAX_PATH * 2) return p;
    return std::wstring(buf, n);
}

static std::vector<std::wstring> FindTextFilesInCwd() {
    static const wchar_t* kExts[] = {
        L".txt", L".md", L".log", L".ini", L".csv", L".json", L".xml",
        L".c", L".h", L".cpp", L".hpp", L".cc", L".py", L".js", L".ts",
        L".html", L".htm", L".css", L".sh", L".bat", L".yaml", L".yml", L".toml",
    };
    std::vector<std::wstring> out;
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(L"*", &fd);
    if (h == INVALID_HANDLE_VALUE) return out;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring lower = fd.cFileName;
        for (auto& c : lower) c = (wchar_t)towlower(c);
        for (auto e : kExts) {
            size_t el = wcslen(e);
            if (lower.size() > el &&
                lower.compare(lower.size() - el, el, e) == 0) {
                out.push_back(GetFullPath(fd.cFileName));
                break;
            }
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return out;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        g_hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL |
            ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | ES_NOHIDESEL,
            0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)editor::ID_EDIT,
            GetModuleHandleW(nullptr), nullptr);
        editor::Attach(hwnd, g_hEdit);
        break;
    case WM_SIZE:
        if (g_hEdit) MoveWindow(g_hEdit, 0, 0, LOWORD(lp), HIWORD(lp), TRUE);
        break;
    case WM_SETFOCUS:
        if (g_hEdit) SetFocus(g_hEdit);
        break;
    case WM_COMMAND:
        if ((HWND)lp == g_hEdit && HIWORD(wp) == EN_CHANGE) {
            editor::OnEditChange();
        } else {
            editor::OnCommand(LOWORD(wp));
        }
        break;
    case WM_CLOSE:
        if (!editor::ConfirmSaveIfDirty()) break;
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::wstring cliPath;
    if (argv && argc >= 2 && argv[1][0]) cliPath = argv[1];
    if (argv) LocalFree(argv);

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"QuietextWindow";
    wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassExW(&wc);

    g_hMain = CreateWindowExW(0, L"QuietextWindow", L"quietext",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, editor::BuildMenu(), hInst, nullptr);
    if (!g_hMain) return 1;

    ShowWindow(g_hMain, nCmdShow);
    UpdateWindow(g_hMain);

    // 启动文件
    if (!cliPath.empty()) {
        editor::OpenPath(cliPath);
    } else {
        auto files = FindTextFilesInCwd();
        if (files.size() == 1) {
            editor::OpenPath(files[0]);
        } else {
            std::wstring picked;
            if (editor::DoOpenDialog(picked)) editor::OpenPath(picked);
            // 取消：编辑器保持未命名空白状态，标题已由 Attach 设置为 "未命名 - quietext"
        }
    }

    // 加速键（Ctrl+Q 走 ID_FILE_CLOSE）
    ACCEL accel[] = {
        { FVIRTKEY | FCONTROL,         'S', editor::ID_FILE_SAVE },
        { FVIRTKEY | FCONTROL,         'O', editor::ID_FILE_OPEN },
        { FVIRTKEY | FCONTROL | FSHIFT,'S', editor::ID_FILE_SAVEAS },
        { FVIRTKEY | FCONTROL,         'N', editor::ID_FILE_NEW },
        { FVIRTKEY | FCONTROL,         'Q', editor::ID_FILE_CLOSE },
        { FVIRTKEY | FCONTROL,         'Z', editor::ID_EDIT_UNDO },
    };
    HACCEL hAccel = CreateAcceleratorTableW(accel,
        (int)(sizeof(accel) / sizeof(accel[0])));

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (!TranslateAcceleratorW(g_hMain, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    if (hAccel) DestroyAcceleratorTable(hAccel);
    return (int)msg.wParam;
}
