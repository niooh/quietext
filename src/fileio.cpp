#include "fileio.h"
#include <limits>

namespace fileio {

namespace {

bool ShowError(HWND owner, const wchar_t* message) {
    MessageBoxW(owner, message, L"quietext", MB_ICONERROR);
    return false;
}

HANDLE OpenFile(const std::wstring& path, DWORD access, DWORD share, DWORD create) {
    return CreateFileW(path.c_str(), access, share, nullptr, create,
                       FILE_ATTRIBUTE_NORMAL, nullptr);
}

} // namespace

std::wstring GetFullPath(const std::wstring& path) {
    wchar_t buffer[MAX_PATH * 2];
    DWORD length = GetFullPathNameW(path.c_str(), MAX_PATH * 2, buffer, nullptr);
    if (length == 0 || length >= MAX_PATH * 2) return path;
    return std::wstring(buffer, length);
}

bool ReadAllBytes(const std::wstring& path, std::vector<BYTE>& out, HWND errorOwner) {
    out.clear();

    HANDLE file = OpenFile(path, GENERIC_READ | FILE_WRITE_ATTRIBUTES,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           OPEN_EXISTING);
    bool canRestore = true;
    if (file == INVALID_HANDLE_VALUE) {
        canRestore = false;
        file = OpenFile(path, GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        OPEN_EXISTING);
    }
    if (file == INVALID_HANDLE_VALUE) return ShowError(errorOwner, L"Cannot open file.");

    FILETIME ct{}, at{}, wt{};
    bool haveTimes = canRestore && GetFileTime(file, &ct, &at, &wt);

    auto finish = [&](bool ok) {
        if (!ok) out.clear();
        if (haveTimes) SetFileTime(file, &ct, &at, &wt);
        CloseHandle(file);
        return ok;
    };

    LARGE_INTEGER fileSize{};
    if (!GetFileSizeEx(file, &fileSize))
        return finish(ShowError(errorOwner, L"Cannot determine file size."));

    if (fileSize.QuadPart < 0 ||
        static_cast<unsigned long long>(fileSize.QuadPart) >
        static_cast<unsigned long long>(std::numeric_limits<size_t>::max()))
        return finish(ShowError(errorOwner, L"File is too large."));

    size_t total = static_cast<size_t>(fileSize.QuadPart);
    out.resize(total);

    for (size_t offset = 0; offset < total; ) {
        size_t remaining = total - offset;
        DWORD chunk = remaining > MAXDWORD ? MAXDWORD : static_cast<DWORD>(remaining);
        DWORD bytesRead = 0;

        if (!ReadFile(file, out.data() + offset, chunk, &bytesRead, nullptr))
            return finish(ShowError(errorOwner, L"Failed to read file."));
        if (bytesRead == 0)
            return finish(ShowError(errorOwner, L"Unexpected end of file."));
        offset += bytesRead;
    }

    return finish(true);
}

bool WriteAllBytes(const std::wstring& path, const std::vector<BYTE>& bytes, HWND errorOwner) {
    bool existed = GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;

    HANDLE file = OpenFile(path,
                           GENERIC_WRITE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                           FILE_SHARE_READ, OPEN_ALWAYS);
    if (file == INVALID_HANDLE_VALUE) return ShowError(errorOwner, L"Cannot open file for saving.");

    FILETIME ct{}, at{}, wt{};
    bool haveTimes = existed && GetFileTime(file, &ct, &at, &wt);

    auto cleanup = [&](bool restoreTimes) {
        if (restoreTimes && haveTimes) SetFileTime(file, &ct, &at, &wt);
        CloseHandle(file);
    };
    auto fail = [&](const wchar_t* msg) { cleanup(true); return ShowError(errorOwner, msg); };

    LARGE_INTEGER zero{};
    if (!SetFilePointerEx(file, zero, nullptr, FILE_BEGIN))
        return fail(L"Cannot seek in file.");

    for (size_t offset = 0; offset < bytes.size(); ) {
        size_t remaining = bytes.size() - offset;
        DWORD chunk = remaining > MAXDWORD ? MAXDWORD : static_cast<DWORD>(remaining);
        DWORD bytesWritten = 0;

        if (!WriteFile(file, bytes.data() + offset, chunk, &bytesWritten, nullptr) || bytesWritten == 0)
            return fail(L"Failed to write file.");
        offset += bytesWritten;
    }

    if (!SetEndOfFile(file)) return fail(L"Failed to resize file.");
    if (!FlushFileBuffers(file)) return fail(L"Failed to flush file.");

    if (haveTimes && !SetFileTime(file, &ct, &at, &wt))
        MessageBoxW(errorOwner, L"File was saved, but its original timestamps could not be restored.",
                    L"quietext", MB_ICONWARNING);
    cleanup(false);
    return true;
}

} // namespace fileio
