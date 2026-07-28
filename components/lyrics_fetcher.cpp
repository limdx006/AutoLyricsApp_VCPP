#include "lyrics_fetcher.h"
#include "time_formatter.h"

#include <cstdio>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Extracts the embedded py_helper.exe to %TEMP%\AutoLyricsApp\ and returns
// the absolute path, or an empty string on failure (caller may fall back
// to running the .py file directly for development convenience).
static string extract_py_helper()
{
    HRSRC hRes = FindResourceW(nullptr, L"PY_HELPER", (LPCWSTR)RT_RCDATA);
    if (!hRes) return {};

    HGLOBAL hLoaded = LoadResource(nullptr, hRes);
    if (!hLoaded) return {};

    void* pData = LockResource(hLoaded);
    DWORD dwSize = SizeofResource(nullptr, hRes);
    if (!pData || !dwSize) return {};

    wchar_t tempPath[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, tempPath))
        return {};

    wstring exeDir = wstring(tempPath) + L"AutoLyricsApp";
    wstring exePath = exeDir + L"\\py_helper.exe";

    CreateDirectoryW(exeDir.c_str(), nullptr);

    FILE* f = nullptr;
    if (_wfopen_s(&f, exePath.c_str(), L"wb") != 0 || !f)
        return {};

    fwrite(pData, 1, dwSize, f);
    fclose(f);

    int len = WideCharToMultiByte(CP_UTF8, 0, exePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    string path(len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, exePath.c_str(), -1, &path[0], len, nullptr, nullptr);
    return path;
}

string get_lyrics(const string& title, const string& artist)
{
    SetEnvironmentVariableW(L"TRACK_TITLE", utf8_to_wide(title).c_str());
    SetEnvironmentVariableW(L"TRACK_ARTIST", utf8_to_wide(artist).c_str());

    // Try the self-contained exe first; fall back to running the .py file
    // directly (useful during development before the exe has been rebuilt).
    string helperPath = extract_py_helper();
    string command;
    if (!helperPath.empty())
    {
        command = "\"" + helperPath + "\"";
    }
    else
    {
        command = "python \"components/lyrics_fetcher.py\"";
    }

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

vector<wstring> translate_lyrics(const vector<wstring>& texts, const string& langCode)
{
    vector<wstring> result;
    if (texts.empty()) return result;

    // ── Build JSON input ──
    json j;
    j["language"] = langCode;
    json linesArr = json::array();
    for (const auto& wtext : texts)
        linesArr.push_back(wide_to_utf8(wtext));
    j["lines"] = linesArr;
    const string jsonInput = j.dump();

    // ── Write JSON to a temp file ──
    wchar_t tempPath[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, tempPath)) return result;

    const wstring tempDir = wstring(tempPath) + L"AutoLyricsApp";
    CreateDirectoryW(tempDir.c_str(), nullptr);

    wchar_t tempFile[MAX_PATH];
    if (!GetTempFileNameW(tempDir.c_str(), L"TRN", 0, tempFile)) return result;

    {
        FILE* f = nullptr;
        if (_wfopen_s(&f, tempFile, L"wb") != 0 || !f)
        {
            DeleteFileW(tempFile);
            return result;
        }
        fwrite(jsonInput.data(), 1, jsonInput.size(), f);
        fclose(f);
    }

    // ── Invoke translator with stdin redirected from the temp file ──
    const string tempFileUtf8 = wide_to_utf8(tempFile);
    const string command = "python components/lyrics_translator.py < \"" + tempFileUtf8 + "\"";

    array<char, 4096> buffer;
    string rawOutput;
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe)
    {
        DeleteFileW(tempFile);
        return result;
    }
    while (fgets(buffer.data(), buffer.size(), pipe))
        rawOutput += buffer.data();
    _pclose(pipe);
    DeleteFileW(tempFile);

    // ── Parse JSON response ──
    try
    {
        const json out = json::parse(rawOutput);
        if (!out.value("success", false))
            return result;

        const auto translated = out["lines"];
        if (!translated.is_array() || translated.empty())
            return result;

        result.reserve(translated.size());
        for (const auto& line : translated)
            result.push_back(utf8_to_wide(line.get<string>()));
    }
    catch (...)
    {
        result.clear();
    }
    return result;
}