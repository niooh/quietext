#include "encoding.h"

#include <climits>
#include <cstring>

namespace {

// UTF-8 and ACP decoding share the same flow; only the code page and flags differ.
bool DecodeMB(const BYTE* data, size_t size, UINT cp, DWORD flags, std::wstring& out) {
    if (size == 0) { out.clear(); return true; }
    if (size > INT_MAX) return false;

    int length = MultiByteToWideChar(cp, flags, reinterpret_cast<LPCCH>(data),
                                     static_cast<int>(size), nullptr, 0);
    if (length <= 0) return false;

    out.resize(length);
    int result = MultiByteToWideChar(cp, flags, reinterpret_cast<LPCCH>(data),
                                     static_cast<int>(size), &out[0], length);
    return result == length;
}

// UTF-8 and ACP encoding share the same flow (BOM handling is layered on top).
bool EncodeMB(const std::wstring& text, UINT cp, DWORD flags, bool checkDefault,
              std::vector<BYTE>& out) {
    if (text.empty()) { out.clear(); return true; }
    if (text.size() > INT_MAX) return false;

    BOOL usedDefault = FALSE;
    int byteCount = WideCharToMultiByte(cp, flags, text.c_str(),
                                        static_cast<int>(text.size()),
                                        nullptr, 0, nullptr,
                                        checkDefault ? &usedDefault : nullptr);
    if (byteCount <= 0 || (checkDefault && usedDefault)) return false;

    out.resize(byteCount);
    usedDefault = FALSE;
    int result = WideCharToMultiByte(cp, flags, text.c_str(),
                                     static_cast<int>(text.size()),
                                     reinterpret_cast<char*>(out.data()), byteCount,
                                     nullptr, checkDefault ? &usedDefault : nullptr);
    return result == byteCount && !(checkDefault && usedDefault);
}

bool EncodeUTF8(const std::wstring& text, bool withBOM, std::vector<BYTE>& out) {
    std::vector<BYTE> body;
    if (!EncodeMB(text, CP_UTF8, WC_ERR_INVALID_CHARS, false, body)) return false;

    out.clear();
    if (withBOM) { out.push_back(0xEF); out.push_back(0xBB); out.push_back(0xBF); }
    out.insert(out.end(), body.begin(), body.end());
    return true;
}

bool DecodeUTF16(const BYTE* data, size_t size, bool littleEndian, std::wstring& out) {
    if (size & 1) return false;

    size_t count = size / 2;
    out.resize(count);
    if (count == 0) return true;

    if (littleEndian) {
        std::memcpy(&out[0], data, size);
    } else {
        for (size_t i = 0; i < count; ++i)
            out[i] = static_cast<wchar_t>((static_cast<unsigned>(data[i * 2]) << 8) |
                                          static_cast<unsigned>(data[i * 2 + 1]));
    }
    return true;
}

bool EncodeUTF16(const std::wstring& text, bool littleEndian, std::vector<BYTE>& out) {
    out.resize(text.size() * 2);
    for (size_t i = 0; i < text.size(); ++i) {
        WORD value = static_cast<WORD>(text[i]);
        if (littleEndian) {
            out[i * 2]     = static_cast<BYTE>(value & 0xFF);
            out[i * 2 + 1] = static_cast<BYTE>(value >> 8);
        } else {
            out[i * 2]     = static_cast<BYTE>(value >> 8);
            out[i * 2 + 1] = static_cast<BYTE>(value & 0xFF);
        }
    }
    return true;
}

} // namespace

bool DetectAndDecode(const std::vector<BYTE>& data, std::wstring& out, Enc& enc) {
    out.clear();

    const BYTE* p = data.data();
    size_t n = data.size();

    if (n >= 3 && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) {
        if (!DecodeMB(p + 3, n - 3, CP_UTF8, MB_ERR_INVALID_CHARS, out)) return false;
        enc = Enc::UTF8BOM;
        return true;
    }

    if (n >= 2 && p[0] == 0xFF && p[1] == 0xFE) {
        if (!DecodeUTF16(p + 2, n - 2, true, out)) return false;
        enc = Enc::UTF16LE;
        return true;
    }

    if (n >= 2 && p[0] == 0xFE && p[1] == 0xFF) {
        if (!DecodeUTF16(p + 2, n - 2, false, out)) return false;
        enc = Enc::UTF16BE;
        return true;
    }

    if (DecodeMB(p, n, CP_UTF8, MB_ERR_INVALID_CHARS, out)) { enc = Enc::UTF8; return true; }
    if (!DecodeMB(p, n, CP_ACP, 0, out)) return false;
    enc = Enc::ACP;
    return true;
}

std::vector<BYTE> EncodeText(const std::wstring& text, Enc enc, bool& ok) {
    std::vector<BYTE> out;
    ok = true;

    switch (enc) {
    case Enc::UTF8:    ok = EncodeUTF8(text, false, out); break;
    case Enc::UTF8BOM: ok = EncodeUTF8(text, true,  out); break;
    case Enc::ACP:     ok = EncodeMB(text, CP_ACP, WC_NO_BEST_FIT_CHARS, true, out); break;

    case Enc::UTF16LE:
    case Enc::UTF16BE: {
        bool le = (enc == Enc::UTF16LE);
        std::vector<BYTE> body;
        ok = EncodeUTF16(text, le, body);
        if (ok) {
            out.reserve(body.size() + 2);
            out.push_back(le ? 0xFF : 0xFE);
            out.push_back(le ? 0xFE : 0xFF);
            out.insert(out.end(), body.begin(), body.end());
        }
        break;
    }
    }
    return out;
}

std::wstring ToCRLF(const std::wstring& text) {
    std::wstring result;
    result.reserve(text.size() + text.size() / 8 + 8);

    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == L'\r') {
            result += L"\r\n";
            if (i + 1 < text.size() && text[i + 1] == L'\n') ++i;
        } else if (text[i] == L'\n') {
            result += L"\r\n";
        } else {
            result += text[i];
        }
    }
    return result;
}

std::wstring CRLFtoLF(const std::wstring& text) {
    std::wstring result;
    result.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == L'\r' && i + 1 < text.size() && text[i + 1] == L'\n') continue;
        result += text[i];
    }
    return result;
}

bool IsPureCRLF(const std::wstring& text) {
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == L'\r') {
            if (i + 1 >= text.size() || text[i + 1] != L'\n') return false;
            ++i;
        } else if (text[i] == L'\n') {
            return false;
        }
    }
    return true;
}
