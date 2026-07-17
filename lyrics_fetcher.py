import sys
import time
import syncedlyrics

MAX_ATTEMPT = 3

def main():
    if len(sys.argv) < 3:
        print("")
        return

    title = sys.argv[1]
    artist = sys.argv[2]

    query = f"{title} {artist}"
    
    for attempt in range(MAX_ATTEMPT):
        try:
            lyrics = syncedlyrics.search(query)
            if lyrics:
                print(lyrics)
                return # Exit the function after successful retrieval
            return  
        except Exception as e:
            last_error = e
            if attempt < MAX_ATTEMPT - 1:
                time.sleep(1)  # Wait for 1 second before retrying
    print(f"ERROR: {e}", file=sys.stderr)

if __name__ == "__main__":
    main()
