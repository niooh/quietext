#pragma once
#include <windows.h>
#include <string>
#include <vector>

namespace fileio {

// Normalize to an absolute path; returns the input on failure.
std::wstring GetFullPath(const std::wstring& path);

// Read all bytes; restores LastAccessTime touched by the read.
bool ReadAllBytes(const std::wstring& path, std::vector<BYTE>& out, HWND errorOwner);

// Write all bytes; preserves CreationTime / LastAccessTime / LastWriteTime
// whenever possible. Newly created files get no timestamp override.
bool WriteAllBytes(const std::wstring& path, const std::vector<BYTE>& data, HWND errorOwner);

}
