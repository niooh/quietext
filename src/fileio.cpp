#include "fileio.h"

namespace fileio {

bool ReadAllBytes(const std::wstring& path, std::vector<BYTE>& out, HWND errorOwner) {
    HANDLE h = CreateFileW(path.c_str(),
                           GENERIC_READ | FILE_WRITE_ATTRIBUTES,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        MessageBoxW(errorOwner, L"Cannot open file.", L"quietext", MB_ICONERROR);
        return false;
    }
    FILETIME ct{}, at{}, wt{};
    GetFileTime(h, &ct, &at, &wt);

    LARGE_INTEGER sz{};
    GetFileSizeEx(h, &sz);
    out.resize((size_t)sz.QuadPart);
    if (!out.empty()) {
        DWORD got = 0;
        ReadFile(h, out.data(), (DWORD)out.size(), &got, nullptr);
    }

    SetFileTime(h, &ct, &at, &wt);  // restore atime touched by the read
    CloseHandle(h);
    return true;
}

bool WriteAllBytes(const std::wstring& path, const std::vector<BYTE>& bytes,
                   HWND errorOwner) {
    bool existed = GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
    HANDLE h = CreateFileW(path.c_str(),
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ, nullptr,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        MessageBoxW(errorOwner,
                    L"Cannot open file for saving (path or permission issue).",
                    L"quietext", MB_ICONERROR);
        return false;
    }

    FILETIME ct{}, at{}, wt{};
    if (existed) GetFileTime(h, &ct, &at, &wt);

    SetFilePointer(h, 0, nullptr, FILE_BEGIN);
    SetEndOfFile(h);
    DWORD written = 0;
    BOOL ok = WriteFile(h, bytes.data(), (DWORD)bytes.size(), &written, nullptr);
    if (!ok || written != bytes.size()) {
        CloseHandle(h);
        MessageBoxW(errorOwner, L"Failed to write file.", L"quietext", MB_ICONERROR);
        return false;
    }

    if (existed) SetFileTime(h, &ct, &at, &wt);  // restore all three timestamps
    CloseHandle(h);
    return true;
}

}