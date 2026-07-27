#include "lyrics_fetcher.h"
#include "time_formatter.h"

#include <cstdio>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Extracts the embedded lyrics_fetcher.py to %TEMP%\AutoLyricsApp\ and returns
// the absolute path, or an empty string on failure (caller may fall back).
static string extract_script_to_temp()
{
    // Locate the embedded resource
    HRSRC hRes = FindResourceW(nullptr, L"LYRICS_FETCHER", (LPCWSTR)RT_RCDATA);
    if (!hRes) return {};

    HGLOBAL hLoaded = LoadResource(nullptr, hRes);
    if (!hLoaded) return {};

    void* pData = LockResource(hLoaded);
    DWORD dwSize = SizeofResource(nullptr, hRes);
    if (!pData || !dwSize) return {};

    // Build %TEMP%\AutoLyricsApp\lyrics_fetcher.py
    wchar_t tempPath[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, tempPath))
        return {};

    wstring scriptDir = wstring(tempPath) + L"AutoLyricsApp";
    wstring scriptPath = scriptDir + L"\\lyrics_fetcher.py";

    CreateDirectoryW(scriptDir.c_str(), nullptr);

    // Always overwrite so the extracted copy matches the embedded one.
    FILE* f = nullptr;
    if (_wfopen_s(&f, scriptPath.c_str(), L"wb") != 0 || !f)
        return {};

    fwrite(pData, 1, dwSize, f);
    fclose(f);

    // Convert back to narrow string for the _popen command line
    int len = WideCharToMultiByte(CP_UTF8, 0, scriptPath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    string path(len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, scriptPath.c_str(), -1, &path[0], len, nullptr, nullptr);
    return path;
}

string get_lyrics(const string& title, const string& artist)
{
    SetEnvironmentVariableW(L"TRACK_TITLE", utf8_to_wide(title).c_str());
    SetEnvironmentVariableW(L"TRACK_ARTIST", utf8_to_wide(artist).c_str());

    // Extract the embedded script to temp, fall back to co-located file
    // (for development convenience when running from the build directory).
    string scriptPath = extract_script_to_temp();
    if (scriptPath.empty())
        scriptPath = "components/Lyrics_fetcher.py";

    string command = "python \"" + scriptPath + "\"";

    array<char, 256> buffer;
    string result;

    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) return "";

    while (fgets(buffer.data(), buffer.size(), pipe))
        result += buffer.data();

    _pclose(pipe);
    return result;
}

// Splits raw LRC text (one "[mm:ss.cc] line text" per line, plus possible
// non-timed metadata lines like "[ar:...]") into sorted, timestamped lines.
static vector<LyricLine> parse_lrc(const string& lrcText)
{
    vector<LyricLine> lines;
    std::istringstream stream(lrcText);
    string rawLine;

    while (std::getline(stream, rawLine))
    {
        if (rawLine.empty() || rawLine.front() != '[')
            continue;

        size_t closeBracket = rawLine.find(']');
        if (closeBracket == string::npos)
            continue;

        float timestamp = parse_lrc_time(rawLine.substr(1, closeBracket - 1));
        if (timestamp < 0.0f)
            continue; // metadata tag (e.g. "[ar:...]"), not a timing tag

        string text = rawLine.substr(closeBracket + 1);
        if (!text.empty() && text.front() == ' ')
            text.erase(0, 1);

        // Trim trailing whitespace/CR (e.g. from CRLF line endings) so a
        // line that's only whitespace is correctly treated as blank below.
        while (!text.empty() && (text.back() == ' ' || text.back() == '\r' || text.back() == '\t'))
            text.pop_back();

        if (text.empty())
            continue; // blank line (instrumental break) -- skip instead of showing an empty highlighted line

        lines.push_back(LyricLine{ timestamp, utf8_to_wide(text) });
    }

    std::sort(lines.begin(), lines.end(), [](const LyricLine& a, const LyricLine& b) {
        return a.timestamp < b.timestamp;
    });

    return lines;
}

LyricsResult fetch_lyrics(const string& title, const string& artist)
{
    LyricsResult result;

    string response = get_lyrics(title, artist);
    if (response.empty())
        return result;

    json j;
    try
    {
        j = json::parse(response);
    }
    catch (...)
    {
        return result; // malformed response from the script
    }

    if (!j.value("success", false))
        return result;

    result.lines = parse_lrc(j.value("lyrics", ""));
    result.success = !result.lines.empty();
    return result;
}