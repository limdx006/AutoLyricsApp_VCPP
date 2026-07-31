import syncedlyrics

title = input("Song title : ").strip()
artist = input("Artist     : ").strip()

query = f"{title} {artist}"

print(f"\nSearching for: {query}")
print("=" * 60)

# Enhanced lyrics
print("\n[Enhanced Search]")
try:
    lyrics = syncedlyrics.search(query, enhanced=True)

    if lyrics:
        print("Found enhanced lyrics!\n")
        print(lyrics)
    else:
        print("No enhanced lyrics found.")
        # print("\n" + "=" * 60)
        # Normal synced lyrics
        # print("\n[Normal Synced Search]")
        # try:
        #     lyrics = syncedlyrics.search(query)

        #     if lyrics:
        #         print("Found synced lyrics!\n")
        #         print(lyrics)
        #     else:
        #         print("No synced lyrics found.")
        # except Exception as e:
        #     print(f"Error: {e}")

except Exception as e:
    print(f"Error: {e}")

