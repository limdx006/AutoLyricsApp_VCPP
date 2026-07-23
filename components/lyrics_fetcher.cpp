#include "lyrics_fetcher.h"
#include "time_formatter.h"

#include <cstdio>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

string get_lyrics(const string& title, const string& artist)
{
    SetEnvironmentVariableW(L"TRACK_TITLE", utf8_to_wide(title).c_str());
    SetEnvironmentVariableW(L"TRACK_ARTIST", utf8_to_wide(artist).c_str());

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