#pragma once

#include <windows.h>
#include <string>
#include <vector>

enum class Enc {
    UTF8,
    UTF8BOM,
    UTF16LE,
    UTF16BE,
    ACP
};

bool DetectAndDecode(
    const std::vector<BYTE>& data,
    std::wstring& out,
    Enc& enc
);

std::vector<BYTE> EncodeText(
    const std::wstring& text,
    Enc enc,
    bool& ok
);

std::wstring ToCRLF(const std::wstring& text);
std::wstring CRLFtoLF(const std::wstring& text);

bool IsPureCRLF(const std::wstring& text);
