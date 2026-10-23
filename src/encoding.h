#pragma once
#include <windows.h>
#include <string>
#include <vector>

enum class Enc { UTF8, UTF8BOM, UTF16LE, UTF16BE, ACP };

// Decode raw bytes; returns the detected encoding
// (UTF-8 BOM / UTF-16 LE/BE BOM / strict UTF-8 / ACP).
Enc DetectAndDecode(const std::vector<BYTE>& data, std::wstring& out);

// Encode with the given encoding. ok=false means some characters
// cannot be represented in that encoding.
std::vector<BYTE> EncodeText(const std::wstring& s, Enc enc, bool& ok);

// Line ending helpers
std::wstring ToCRLF(const std::wstring& s);     // normalize to \r\n (Edit control format)
std::wstring CRLFtoLF(const std::wstring& s);   // \r\n -> \n
bool         IsPureCRLF(const std::wstring& s); // true only if every line break is CRLF and at least one exists
