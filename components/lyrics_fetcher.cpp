#include "lyrics_fetcher.h"

#include <windows.h>
#include <cstdio>

string get_lyrics(const string& title, const string& artist)
{
    // Convert UTF-8 std::string -> UTF-16 for SetEnvironmentVariableW
    auto toWide = [](const string& s) -> wstring {
        if (s.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        wstring w(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], len);
        return w;
    };

    SetEnvironmentVariableW(L"TRACK_TITLE", toWide(title).c_str());
    SetEnvironmentVariableW(L"TRACK_ARTIST", toWide(artist).c_str());

    string command = "python lyrics_fetcher.py";

    array<char, 256> buffer;
    string result;

    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) return "";

    while (fgets(buffer.data(), buffer.size(), pipe))
        result += buffer.data();

    _pclose(pipe);
    return result;
}