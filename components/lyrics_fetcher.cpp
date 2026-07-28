#include "lyrics_fetcher.h"
#include "time_formatter.h"

#include <cstdio>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ── Subprocess runner (no console window) ──

// Runs a command line via CreateProcessW with CREATE_NO_WINDOW so no
// terminal flashes for the child.  Captures both stdout and stderr into the
// returned string.  Returns empty string on failure.
static string run_no_window(const wstring& command)
{
    string result;
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hRead, hWrite;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
        return result;

    // Read end is NOT inherited — only the write end goes to the child.
    if (!SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0))
    {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return result;
    }

    PROCESS_INFORMATION pi = {};
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError  = hWrite;   // stderr → same pipe as stdout

    // Provide a null stdin pipe so the child doesn't inherit an invalid
    // handle (parent is a GUI app with no console handles).
    HANDLE hStdinRead, hStdinWrite;
    if (!CreatePipe(&hStdinRead, &hStdinWrite, &sa, 0))
    {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return result;
    }
    CloseHandle(hStdinWrite); // write end closed — child gets EOF on stdin
    si.hStdInput = hStdinRead;

    // Build a writable command line (CreateProcessW modifies it in place).
    wstring mutableCmd = command;

    if (!CreateProcessW(
            nullptr,              // app name (use command line)
            &mutableCmd[0],       // command line
            nullptr,              // process attributes
            nullptr,              // thread attributes
            TRUE,                 // inherit handles (needed for the pipe)
            CREATE_NO_WINDOW,     // no console window for the child
            nullptr,              // environment (inherit parent's)
            nullptr,              // current directory (inherit parent's)
            &si,
            &pi))
    {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        CloseHandle(hStdinRead);
        return result;
    }

    // The write end is owned by the child now — close ours so ReadFile
    // doesn't hang waiting for more data after the child exits.
    CloseHandle(hWrite);
    CloseHandle(hStdinRead);  // stdin read end owned by the child

    char buf[4096];
    DWORD bytesRead;
    while (ReadFile(hRead, buf, sizeof(buf) - 1, &bytesRead, nullptr) && bytesRead > 0)
    {
        buf[bytesRead] = '\0';
        result += buf;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hRead);
    return result;
}

// ── Embedded resource extraction ──

// Extracts a named RCDATA resource to %TEMP%\AutoLyricsApp\<fileName> and
// returns the absolute path (empty string on failure).
static string extract_resource(const wchar_t* resName, const wchar_t* fileName)
{
    HRSRC hRes = FindResourceW(nullptr, resName, (LPCWSTR)RT_RCDATA);
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
    wstring exePath = exeDir + L"\\" + fileName;

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

static string extract_py_helper()
{
    return extract_resource(L"PY_HELPER", L"py_helper.exe");
}

static string extract_py_translator()
{
    return extract_resource(L"PY_TRANSLATOR", L"py_translator.exe");
}

// ── Lyrics fetching ──

string get_lyrics(const string& title, const string& artist)
{
    SetEnvironmentVariableW(L"TRACK_TITLE", utf8_to_wide(title).c_str());
    SetEnvironmentVariableW(L"TRACK_ARTIST", utf8_to_wide(artist).c_str());

    // Try the self-contained exe first; fall back to running the .py file
    // directly (useful during development before the exe has been rebuilt).
    string helperPath = extract_py_helper();
    wstring command;
    if (!helperPath.empty())
    {
        command = L"\"" + utf8_to_wide(helperPath) + L"\"";
    }
    else
    {
        command = L"python \"components/lyrics_fetcher.py\"";
    }

    return run_no_window(command);
}

// ── LRC parsing ──

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

// ── Translation / romanization ──

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

    // ── Invoke translator with the temp file path via env var ──
    // Passing it as a command-line argument triggers cmd.exe /S /c outer-
    // quote stripping which leaves stray " inside the middle of paths.
    // An environment variable avoids shell parsing altogether.
    SetEnvironmentVariableW(L"TRANSLATOR_INPUT_FILE", tempFile);

    string translatorPath = extract_py_translator();
    wstring command;
    if (!translatorPath.empty())
    {
        command = L"\"" + utf8_to_wide(translatorPath) + L"\"";
    }
    else
    {
        command = L"python \"components/lyrics_translator.py\"";
    }

    string rawOutput = run_no_window(command);
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