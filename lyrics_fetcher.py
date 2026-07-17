import os
import sys
import time
from flask import json
import syncedlyrics

sys.stdout.reconfigure(encoding="utf-8")
sys.stderr.reconfigure(encoding="utf-8")

MAX_ATTEMPT = 3

def main():
    title = os.environ.get("TRACK_TITLE", "")
    artist = os.environ.get("TRACK_ARTIST", "")
    if not title:
        print("")
        return

    query = f"{title} {artist}"
    
    for attempt in range(MAX_ATTEMPT):
        try:
            lyrics = syncedlyrics.search(query)
            print(json.dumps({
                "success": True,
                "title": title,
                "artist": artist,
                "lyrics": lyrics or ""}
                , ensure_ascii=False))
            return # Exit the function after retrieval
        except Exception as e:
            last_error = e
            if attempt < MAX_ATTEMPT - 1:
                time.sleep(1)  # Wait for 1 second before retrying
    print(json.dumps({
        "success": False,
        "title": title,
        "artist": artist,
        "message": str(last_error)}
        , ensure_ascii=False))

if __name__ == "__main__":
    main()
