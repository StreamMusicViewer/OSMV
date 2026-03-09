#pragma once
#include <QString>

/// Snapshot of the currently playing track.
struct SongInfo {
    QString title;
    QString artist;
    QString album;
    QString artUrl;          // file:// or http(s):// — empty if none
    QString thumbnailB64;    // base64-encoded image — filled by App
    QString status;          // "playing" | "paused" | "stopped" | "closed"
    bool    isPlaying = false;
};

/// Platform-agnostic interface for reading the current media session.
class IMediaProvider {
public:
    virtual ~IMediaProvider() = default;

    /// Returns the current song info (blocking, fast — called every ~1s).
    virtual SongInfo currentSong() = 0;

    /// Factory: returns a platform-specific implementation.
    static IMediaProvider* create();
};
