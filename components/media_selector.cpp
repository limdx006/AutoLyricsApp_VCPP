#include "media_selector.h"
#include "log_viewer.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace media_selector {

    // Position tracking

    static constexpr double POSITION_DELTA_THRESHOLD = 0.3;       // seconds
    static constexpr unsigned long long POSITION_MIN_INTERVAL_MS = 400;

    struct PositionSample {
        double position = 0.0;
        unsigned long long tick_ms = 0;
    };

    // Key: app_id + "|" + title
    static std::unordered_map<string, PositionSample> s_position_history;
    static std::mutex s_pos_mutex;

    static bool check_position_moving(const string& track_key, double current_pos)
    {
        unsigned long long now = GetTickCount64();
        std::lock_guard<std::mutex> lock(s_pos_mutex);

        auto it = s_position_history.find(track_key);
        if (it != s_position_history.end())
        {
            if (now - it->second.tick_ms >= POSITION_MIN_INTERVAL_MS)
            {
                double delta = current_pos - it->second.position;
                it->second.position = current_pos;
                it->second.tick_ms = now;
                return delta >= POSITION_DELTA_THRESHOLD;
            }
            return false;
        }

        s_position_history[track_key] = { current_pos, now };
        return false;
    }

    //  Lyrics probe cache
    // -1 = not yet known, 0 = not found, 1 = found
    struct SongKey {
        string title;
        string artist;
        bool operator==(const SongKey& o) const {
            return title == o.title && artist == o.artist;
        }
    };
    struct SongKeyHash {
        size_t operator()(const SongKey& k) const {
            return std::hash<string>()(k.title) ^ (std::hash<string>()(k.artist) << 1);
        }
    };

    static std::unordered_map<SongKey, int, SongKeyHash> s_probe_cache;
    static std::mutex s_probe_mutex;

    // Fetch coordination
    static std::unordered_set<string> s_fetching_songs;
    static std::mutex s_fetch_mutex;

    // State
    static bool s_debug = false;
    static int  s_current_score = 0;
    static bool s_dirty = true;

    // s_first_pass: on the first call after a song change, score purely by
    // metadata (ignore probe cache).  After the primary lyrics fetch completes
    // the flag is reset so subsequent calls incorporate probe results.
    // This ensures the best session is selected before any lyrics search
    // influences the scoring, and then re-scored once lyrics are found.
    static bool s_first_pass = true;

    //  String helpers
    static string to_lower(const string& s)
    {
        string r = s;
        // Cast through unsigned char to avoid undefined behaviour for
        // negative-chars in MSVC's ::tolower.
        std::transform(r.begin(), r.end(), r.begin(),
            [](unsigned char c) { return static_cast<char>(::tolower(c)); });
        return r;
    }

    template<size_t N>
    static bool contains_any(const string& s, const char* (&patterns)[N])
    {
        string low = to_lower(s);
        for (size_t i = 0; i < N; ++i)
        {
            if (low.find(patterns[i]) != string::npos)
                return true;
        }
        return false;
    }

    template<size_t N>
    static bool equals_any(const string& s, const char* (&values)[N])
    {
        string low = to_lower(s);
        for (size_t i = 0; i < N; ++i)
        {
            if (low == values[i])
                return true;
        }
        return false;
    }


    //  App classification
    static bool is_blacklisted(const string& app_id)
    {
        static const char* patterns[] = {
            "teams", "zoom", "discord", "slack", "skype", "obs",
            "mpc-hc", "vlc", "wmplayer", "riot", "douyin"
        };
        return contains_any(app_id, patterns);
    }

    static bool is_music_app(const string& app_id)
    {
        static const char* patterns[] = {
            "spotify", "tidal", "applemusic", "musicbee", "foobar",
            "aimp", "winamp", "mediamonkey", "amazon music", "deezer",
            "soundcloud", "ytmusic"
        };
        return contains_any(app_id, patterns);
    }

    static bool is_browser(const string& app_id)
    {
        static const char* patterns[] = {
            "chrome", "msedge", "firefox", "opera", "brave", "vivaldi"
        };
        return contains_any(app_id, patterns);
    }

    static bool is_junk_artist(const string& artist)
    {
        static const char* values[] = {
            "youtube", "unknown", "chrome", "microsoft edge", "firefox",
            "opera", "brave", "vivaldi", "windows"
        };
        return equals_any(artist, values);
    }

    // Title-music likelihood
    // Patterns that suggest this title is a video / non-music content.
    static const char* VIDEO_PATTERNS[] = {
        "episode",    "full hd",    "live stream",  "breaking news",
        "tutorial",   "reaction",   "vlog",         "part ",
        "ep.",        " season ",   "review",       "how to",
        "full movie", "gameplay",
        // CJK patterns
        "\xe7\x9b\xb4\xe6\x92\xad",             // 直播
        "\xe5\xaf\xa6\xe6\xb3\x81",             // 實況
        "\xe6\x81\x90\xe6\x80\x96",             // 恐怖
        "\xe6\x96\xb0\xe8\x81\x9e",             // 新聞
        "\xe6\x94\xbb\xe7\x95\xa5",             // 攻略
        "\xe6\x95\x99\xe5\xad\xb8",             // 教學
        "\xe5\xae\x8c\xe6\x95\xb4\xe7\x89\x88", // 完整版
        "\xe6\x8a\x96\xe9\x9f\xb3",             // 抖音
    };

    static int title_music_delta(const string& title, vector<string>& reasons)
    {
        if (title.empty())
        {
            reasons.push_back("-50 no title");
            return -50;
        }

        int delta = 0;
        string low = to_lower(title);

        // Video / non-music content patterns
        for (size_t i = 0; i < sizeof(VIDEO_PATTERNS) / sizeof(VIDEO_PATTERNS[0]); ++i)
        {
            if (low.find(VIDEO_PATTERNS[i]) != string::npos)
            {
                delta -= 30;
                reasons.push_back(string("-30 title pattern '") + VIDEO_PATTERNS[i] + "'");
                break;
            }
        }

        // Excessive punctuation
        int punct = 0;
        for (char c : title)
        {
            if (c == '!' || c == '?') ++punct;
        }
        if (punct >= 2)
        {
            delta -= 15;
            reasons.push_back("-15 excessive punctuation");
        }

        // Suspiciously long title
        size_t len = title.length();
        if (len > 60)
        {
            delta -= 30;
            reasons.push_back(string("-30 long title (") + std::to_string(len) + " chars)");
        }

        return delta;
    }

    //  Lyrics-probe score contribution
    static constexpr int LYRICS_FOUND_BONUS   = 60;
    static constexpr int LYRICS_MISSING_PENALTY = 40;

    static int lyrics_probe_delta(const string& title, const string& artist,  vector<string>& reasons)
    {
        if (title.empty()) return 0;

        // During the initial metadata-only pass, ignore the probe cache so
        // that old "no lyrics" results from previous songs don't bias the
        // session selection.  After the primary fetch completes the flag is
        // reset and subsequent calls will use the cache.
        if (s_first_pass) return 0;

        SongKey key{ title, artist };
        std::lock_guard<std::mutex> lock(s_probe_mutex);
        auto it = s_probe_cache.find(key);
        if (it != s_probe_cache.end())
        {
            if (it->second == 1)
            {
                reasons.push_back(string("+") + std::to_string(LYRICS_FOUND_BONUS)
                                  + " lyrics found");
                return LYRICS_FOUND_BONUS;
            }
            else if (it->second == 0)
            {
                reasons.push_back(string("-") + std::to_string(LYRICS_MISSING_PENALTY) + " no lyrics");
                return -LYRICS_MISSING_PENALTY;
            }
        }
        return 0;   // not yet probed
    }

    //  Per-session scoring
    // Internal result with score breakdown
    struct ScoredSession {
        string app_id;
        string title;
        string artist;
        string album_title;
        bool   has_thumbnail = false;
        double position  = 0.0;
        double duration  = 0.0;
        bool   is_playing = false;
        bool   is_paused  = false;
        int    score     = 0;
        vector<string> reasons;
    };

    static ScoredSession rate_session(
        const string& app_id,
        const string& title,
        const string& artist,
        const string& album_title,
        bool has_thumbnail,
        bool is_playing,
        bool is_paused,
        bool position_moving,
        double duration)
    {
        ScoredSession s;
        s.app_id        = app_id;
        s.title         = title;
        s.artist        = artist;
        s.album_title   = album_title;
        s.has_thumbnail = has_thumbnail;
        s.is_playing    = is_playing;
        s.is_paused     = is_paused;

        auto& r = s.reasons;

        // Playback status
        if (is_playing)
        {
            s.score += 100;
            r.push_back("+100 playing");
        }
        else if (is_paused)
        {
            s.score += 20;
            r.push_back("+20 paused");
        }

        // Artist
        if (!artist.empty())
        {
            s.score += 40;
            r.push_back("+40 artist present");
            if (is_junk_artist(artist))
            {
                s.score -= 30;
                r.push_back("-30 junk artist");
            }
        }

        // Album title
        if (!album_title.empty())
        {
            s.score += 20;
            r.push_back("+20 album present");
        }

        // Thumbnail
        if (has_thumbnail)
        {
            s.score += 20;
            r.push_back("+20 thumbnail present");
        }

        // Position moving
        if (position_moving)
        {
            s.score += 30;
            r.push_back("+30 position moving");
        }

        // App-type classification
        if (is_music_app(app_id))
        {
            s.score += 60;
            r.push_back("+60 known music app");
        }

        if (is_browser(app_id))
        {
            s.score -= 25;
            r.push_back("-25 browser");

            // Browser offsets when enriched metadata is present
            if (!artist.empty() && !is_junk_artist(artist))
            {
                s.score += 20;
                r.push_back("+20 browser has real artist");
            }
            if (has_thumbnail)
            {
                s.score += 15;
                r.push_back("+15 browser has thumbnail");
            }
            if (!album_title.empty())
            {
                s.score += 15;
                r.push_back("+15 browser has album");
            }
        }

        // Title analysis
        s.score += title_music_delta(title, r);

        // Lyrics probe (cached result)
        s.score += lyrics_probe_delta(title, artist, r);

        // Duration filter: skip very short clips (< 10 s) and penalise
        // non-music long-form content (> 10 min).
        if (duration > 0.0 && duration < 10.0)
        {
            s.score -= 50;
            r.push_back("-50 duration < 10s");
        }
        else if (duration > 600.0)
        {
            s.score -= 30;
            r.push_back("-30 duration > 10min");
        }

        // Blacklist
        if (is_blacklisted(app_id))
        {
            s.score -= 100;
            r.push_back("-100 blacklisted app");
        }

        return s;
    }

    //  Public API
    void initialize()
    {
        s_dirty = true;
        s_current_score = 0;
    }

    void invalidate()
    {
        s_dirty = true;
    }

    int get_current_score()
    {
        return s_current_score;
    }

    // Fetch coordination

    bool try_claim_fetch(const string& title, const string& artist)
    {
        string key = title + "||" + artist;
        std::lock_guard<std::mutex> lock(s_fetch_mutex);
        return s_fetching_songs.insert(key).second;
    }

    void release_fetch(const string& title, const string& artist)
    {
        string key = title + "||" + artist;
        std::lock_guard<std::mutex> lock(s_fetch_mutex);
        s_fetching_songs.erase(key);
    }

    // Probe cache

    void cache_probe_result(const string& title, const string& artist, bool found)
    {
        SongKey key{ title, artist };
        {
            std::lock_guard<std::mutex> lock(s_probe_mutex);
            s_probe_cache[key] = found ? 1 : 0;
        }
        s_dirty = true;   // trigger re-score on next poll
    }

    // Called after the primary lyrics fetch completes.  Resets the first-pass
    // flag so subsequent calls to get_best_session() incorporate probe cache
    // results into the scoring.  Also clears s_dirty since we are about to
    // trigger a fresh scoring pass below (avoid double-trigger from here).
    void finalize_primary_fetch()
    {
        s_first_pass = false;
        s_dirty = true;
    }

    //  get_best_session — main entry point
    MediaSessionInfo get_best_session(
        std::vector<std::pair<string, string>>* all_sessions)
    {
        // Enumerate WinRT sessions 

        // init_apartment is safe to call multiple times.
        winrt::init_apartment();

        MediaSessionInfo result;

        auto manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
        if (!manager)
        {
            if (s_dirty)
                log_viewer::log("[Selector] no manager\n");
            s_current_score = 0;
            return result;
        }

        auto sessions = manager.GetSessions();
        if (!sessions || sessions.Size() == 0)
        {
            if (s_dirty)
                log_viewer::log("[Selector] 0 active sessions\n");
            s_current_score = 0;
            return result;
        }

        // Score each session 

        vector<ScoredSession> scored;
        if (all_sessions)
            all_sessions->clear();

        for (uint32_t i = 0; i < sessions.Size(); ++i)
        {
            auto sess = sessions.GetAt(i);
            if (!sess) continue;

            try
            {
                // App ID
                wstring appIdW(sess.SourceAppUserModelId().c_str());
                string appId = wide_to_utf8(appIdW);

                // Media properties
                auto props = sess.TryGetMediaPropertiesAsync().get();
                if (!props) continue;

                string title   = wide_to_utf8(props.Title().c_str());
                string artist  = wide_to_utf8(props.Artist().c_str());
                string album   = wide_to_utf8(props.AlbumTitle().c_str());
                bool hasThumb  = (props.Thumbnail() != nullptr);

                // Playback info
                auto pb = sess.GetPlaybackInfo();
                auto status = pb ? pb.PlaybackStatus()
                    : GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped;
                bool isPlaying = (status == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing);
                bool isPaused  = (status == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused);

                // Timeline
                auto timeline = sess.GetTimelineProperties();
                double pos = 0.0, dur = 0.0;
                if (timeline)
                {
                    pos = timeline.Position().count() / 10000000.0;
                    dur = timeline.EndTime().count() / 10000000.0;
                }

                // Position-moving detection
                string trackKey = appId + "|" + title;
                bool moving = check_position_moving(trackKey, pos);

                // Score
                ScoredSession ss = rate_session(
                    appId, title, artist, album,
                    hasThumb, isPlaying, isPaused, moving, dur);
                ss.position = pos;
                ss.duration = dur;

                scored.push_back(ss);

                if (all_sessions)
                    all_sessions->emplace_back(title, artist);
            }
            catch (...)
            {
                continue;
            }
        }

        if (scored.empty())
        {
            s_current_score = 0;
            return result;
        }

        // Pick the highest-scoring session

        const auto best = std::max_element(scored.begin(), scored.end(),
            [](const ScoredSession& a, const ScoredSession& b) {
                return a.score < b.score;
            });

        s_current_score = best->score;

        // Logging
        // Always show the session count and winner on a re-score so the
        // user can see what the selector picked and how many sessions are in play.
        if (s_dirty)
        {
            string title_label = best->title.length() > 40
                ? best->title.substr(0, 40) + ".."
                : best->title;
            log_viewer::log("[Selector] %zu active session(s) | %s '%s' (%d)\n",
                            scored.size(),
                            best->app_id.c_str(),
                            title_label.c_str(),
                            best->score);
        }
        if (s_debug && s_dirty)
        {
            for (const auto& ss : scored)
            {
                string t = ss.title.length() > 40
                    ? ss.title.substr(0, 40) + ".."
                    : ss.title;
                log_viewer::log("  %s | '%s' | score: %d\n",
                                ss.app_id.c_str(), t.c_str(), ss.score);
                for (const auto& r : ss.reasons)
                    log_viewer::log("    %s\n", r.c_str());
            }
        }
        s_dirty = false;

        // Floor check
        constexpr int SCORE_FLOOR = -50;
        if (best->score < SCORE_FLOOR)
        {
            s_current_score = 0;
            return result;
        }

        // Build MediaSessionInfo
        result.is_success     = true;
        result.title          = best->title;
        result.artist         = best->artist;
        result.album_title    = best->album_title;
        result.app_id         = best->app_id;
        result.position       = best->position;
        result.duration       = best->duration;
        result.is_playing     = best->is_playing;
        result.has_thumbnail  = best->has_thumbnail;
        result.score          = best->score;

        return result;
    }

    // log_session_details 

    void log_session_details()
    {
        s_debug = true;
        s_dirty = true;
        get_best_session();
        s_debug = false;
    }
}
