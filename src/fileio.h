#pragma once
#include <windows.h>
#include <string>
#include <vector>

namespace fileio {

// Read all bytes; restores LastAccessTime touched by the read.
bool ReadAllBytes(const std::wstring& path, std::vector<BYTE>& out,
                  HWND errorOwner);

// Write all bytes; preserves CreationTime / LastAccessTime / LastWriteTime.
// If the file does not exist, it is created without setting timestamps.
bool WriteAllBytes(const std::wstring& path, const std::vector<BYTE>& data,
                   HWND errorOwner);

}