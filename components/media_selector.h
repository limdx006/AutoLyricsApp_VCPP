#pragma once
#include "common.h"
#include "media_session.h"
#include <utility>
#include <vector>

// Scores every active WinRT media session against music-likeness heuristics
// and returns the session most likely to be playing a song.  The caller
// (timeline_tracker) uses the result as the primary lyrics-display source.
//
// A background lyrics-probe cache is maintained: when a probe result arrives
// it adjusts the score of that session on the next poll (lyrics-found bonus
// vs. no-lyrics penalty).  Fetch coordination via try_claim_fetch /
// release_fetch prevents the same (title, artist) from being fetched twice.
namespace media_selector {
    // Must be called once at startup.
    void initialize();

    // Enumerate all active media sessions, score each, and return the best.
    // If all_sessions is provided it is filled with (title, artist) pairs
    // for every active session so the caller can schedule background probes.
    MediaSessionInfo get_best_session(
        std::vector<std::pair<string, string>>* all_sessions = nullptr);

    // Log the full scoring breakdown for all sessions to the log viewer.
    void log_session_details();

    // Force a full re-score on the next get_best_session() call.
    void invalidate();

    // Score of the session returned by the last get_best_session() call.
    int get_current_score();

    // ── Fetch coordination (shared with timeline_tracker) ──

    // Try to claim the given (title, artist) for a fetch / probe.
    // Returns false if another thread has already claimed it.
    bool try_claim_fetch(const string& title, const string& artist);

    // Release a previously claimed (title, artist).
    void release_fetch(const string& title, const string& artist);

    // Store a probe result in the internal cache so subsequent scoring
    // rounds can apply the lyrics-found bonus or no-lyrics penalty.
    void cache_probe_result(const string& title, const string& artist, bool found);

    // Call after the primary lyrics fetch completes (success or failure).
    // Resets the first-pass metadata-only flag so probe cache results are
    // incorporated into subsequent scoring passes.
    void finalize_primary_fetch();
}
