#pragma once
#include <windows.h>
#include <string>
#include <vector>

namespace fileio {

// 读取全部字节；读后立即恢复 LastAccessTime（读操作会刷新 atime）。
bool ReadAllBytes(const std::wstring& path, std::vector<BYTE>& out,
                  HWND errorOwner);

// 写入全部字节；保留 CreationTime / LastAccessTime / LastWriteTime。
// 若文件不存在则仅创建，不设定时间。
bool WriteAllBytes(const std::wstring& path, const std::vector<BYTE>& data,
                   HWND errorOwner);

}
