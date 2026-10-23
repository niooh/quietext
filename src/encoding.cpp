#include "encoding.h"
#include <cstring>

namespace {

bool TryDecodeUTF8(const BYTE* p, size_t n, std::wstring& out) {
    if (n == 0) { out.clear(); return true; }
    int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                   (LPCCH)p, (int)n, nullptr, 0);
    if (wlen == 0) return false;
    out.resize(wlen);
    MultiByteToWideChar(CP_UTF8, 0, (LPCCH)p, (int)n, &out[0], wlen);
    return true;
}

void DecodeACP(const BYTE* p, size_t n, std::wstring& out) {
    if (n == 0) { out.clear(); return; }
    int wlen = MultiByteToWideChar(CP_ACP, 0, (LPCCH)p, (int)n, nullptr, 0);
    out.resize(wlen > 0 ? wlen : 0);
    if (wlen > 0)
        MultiByteToWideChar(CP_ACP, 0, (LPCCH)p, (int)n, &out[0], wlen);
}

void DecodeUTF16(const BYTE* p, size_t nbytes, bool le, std::wstring& out) {
    size_t n = nbytes / 2;
    out.resize(n);
    if (n == 0) return;
    if (le) {
        std::memcpy(&out[0], p, n * 2);
    } else {
        for (size_t i = 0; i < n; ++i)
            out[i] = (wchar_t)(((unsigned)p[2 * i] << 8) | p[2 * i + 1]);
    }
}

bool EncodeUTF8(const std::wstring& s, bool bom, std::vector<BYTE>& out) {
    int n = s.empty() ? 0 :
        WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(),
                            nullptr, 0, nullptr, nullptr);
    if (!s.empty() && n == 0) return false;
    out.resize(n + (bom ? 3 : 0));
    size_t off = 0;
    if (bom) { out[0] = 0xEF; out[1] = 0xBB; out[2] = 0xBF; off = 3; }
    if (n > 0)
        WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(),
                            (char*)out.data() + off, n, nullptr, nullptr);
    return true;
}

bool EncodeACP(const std::wstring& s, std::vector<BYTE>& out) {
    if (s.empty()) { out.clear(); return true; }
    int n = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS,
                                s.c_str(), (int)s.size(),
                                nullptr, 0, nullptr, nullptr);
    if (n == 0) return false;
    std::vector<BYTE> tmp(n);
    BOOL usedDefault = FALSE;
    WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS,
                        s.c_str(), (int)s.size(),
                        (char*)tmp.data(), n, nullptr, &usedDefault);
    if (usedDefault) return false;   // unrepresentable character
    out.swap(tmp);
    return true;
}

bool EncodeUTF16(const std::wstring& s, bool le, std::vector<BYTE>& out) {
    out.resize(s.size() * 2);
    for (size_t i = 0; i < s.size(); ++i) {
        WORD w = (WORD)s[i];
        if (le) { out[2*i] = (BYTE)(w & 0xFF); out[2*i+1] = (BYTE)(w >> 8); }
        else    { out[2*i] = (BYTE)(w >> 8);   out[2*i+1] = (BYTE)(w & 0xFF); }
    }
    return true;
}

} // namespace

Enc DetectAndDecode(const std::vector<BYTE>& data, std::wstring& out) {
    if (data.size() >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        TryDecodeUTF8(data.data() + 3, data.size() - 3, out);
        return Enc::UTF8BOM;
    }
    if (data.size() >= 2 && data[0] == 0xFF && data[1] == 0xFE) {
        DecodeUTF16(data.data() + 2, data.size() - 2, true, out);
        return Enc::UTF16LE;
    }
    if (data.size() >= 2 && data[0] == 0xFE && data[1] == 0xFF) {
        DecodeUTF16(data.data() + 2, data.size() - 2, false, out);
        return Enc::UTF16BE;
    }
    if (TryDecodeUTF8(data.data(), data.size(), out))
        return Enc::UTF8;
    DecodeACP(data.data(), data.size(), out);
    return Enc::ACP;
}

std::vector<BYTE> EncodeText(const std::wstring& s, Enc enc, bool& ok) {
    std::vector<BYTE> out;
    ok = true;
    switch (enc) {
    case Enc::UTF8:    ok = EncodeUTF8(s, false, out); break;
    case Enc::UTF8BOM: ok = EncodeUTF8(s, true,  out); break;
    case Enc::UTF16LE: {
        out = {0xFF, 0xFE};
        std::vector<BYTE> t; ok = EncodeUTF16(s, true, t);
        out.insert(out.end(), t.begin(), t.end());
        break;
    }
    case Enc::UTF16BE: {
        out = {0xFE, 0xFF};
        std::vector<BYTE> t; ok = EncodeUTF16(s, false, t);
        out.insert(out.end(), t.begin(), t.end());
        break;
    }
    case Enc::ACP: ok = EncodeACP(s, out); break;
    }
    return out;
}

std::wstring ToCRLF(const std::wstring& s) {
    std::wstring r;
    r.reserve(s.size() + s.size() / 8 + 8);
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == L'\r') {
            r += L"\r\n";
            if (i + 1 < s.size() && s[i + 1] == L'\n') ++i;
        } else if (s[i] == L'\n') {
            r += L"\r\n";
        } else {
            r += s[i];
        }
    }
    return r;
}

std::wstring CRLFtoLF(const std::wstring& s) {
    std::wstring r;
    r.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == L'\r' && i + 1 < s.size() && s[i + 1] == L'\n') continue;
        r += s[i];
    }
    return r;
}

bool IsPureCRLF(const std::wstring& s) {
    bool hasCRLF = false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == L'\r') {
            if (i + 1 < s.size() && s[i + 1] == L'\n') { hasCRLF = true; ++i; }
            else return false;  // lone CR
        } else if (s[i] == L'\n') {
            return false;  // lone LF
        }
    }
    return hasCRLF;  // false when there are no line breaks
}
