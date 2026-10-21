#pragma once
#include <windows.h>
#include <string>
#include <vector>

enum class Enc { UTF8, UTF8BOM, UTF16LE, UTF16BE, ACP };

// 从原始字节解码；返回检测到的编码（UTF-8 BOM / UTF-16 LE/BE BOM / 严格 UTF-8 / ACP）。
Enc DetectAndDecode(const std::vector<BYTE>& data, std::wstring& out);

// 按指定编码编码。ok=false 表示有字符在当前编码下无法表示。
std::vector<BYTE> EncodeText(const std::wstring& s, Enc enc, bool& ok);

// 行尾处理
std::wstring ToCRLF(const std::wstring& s);       // 统一成 \r\n（Edit 控件内部格式）
std::wstring CRLFtoLF(const std::wstring& s);     // \r\n -> \n
bool         IsPureCRLF(const std::wstring& s);   // 仅当所有换行都是 CRLF 且至少有一个，返回 true
