#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <windows.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>

using std::string;
using std::wstring;
using std::cout;
using std::wcout;
using std::cin;
using std::vector;
using std::array;
using std::to_string;
using winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager;
using winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus;
using winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession;

// Decodes a UTF-8 std::string into a UTF-16 std::wstring. Do NOT use
// wstring(s.begin(), s.end()) for this -- it widens each byte 1:1 instead
// of decoding multi-byte UTF-8 sequences, corrupting non-ASCII text.
inline wstring utf8_to_wide(const string& s)
{
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) return L"";
    wstring w(len - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], len);
    return w;
}