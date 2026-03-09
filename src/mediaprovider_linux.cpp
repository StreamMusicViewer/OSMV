/// mediaprovider_linux.cpp
/// Linux implementation of IMediaProvider using `playerctl` CLI via QProcess.
/// No D-Bus library required — playerctl abstracts it.

#include "mediaprovider.h"
#include <QFile>
#include <QProcess>

// ── Helper
// ────────────────────────────────────────────────────────────────────
static QString run(const QStringList &args) {
  QProcess p;
  p.start("playerctl", args);
  if (!p.waitForFinished(2000)) {
    p.kill();
    return {};
  }
  QString out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
  if (out == "No players found")
    return {};
  return out;
}

// ── Linux implementation
// ──────────────────────────────────────────────────────
class LinuxMediaProvider : public IMediaProvider {
public:
  SongInfo currentSong() override {
    SongInfo info;

    QString status = run({"status"});
    if (status.isEmpty()) {
      info.status = "closed";
      return info;
    }

    info.status = status.toLower();
    info.isPlaying = (info.status == "playing");

    info.title = run({"metadata", "title"});
    info.artist = run({"metadata", "artist"});
    info.album = run({"metadata", "album"});
    info.artUrl = run({"metadata", "mpris:artUrl"});

    if (info.title.isEmpty())
      info.title = "Unknown Title";
    if (info.artist.isEmpty())
      info.artist = "Unknown Artist";

    return info;
  }
};

// ── Factory
// ───────────────────────────────────────────────────────────────────
IMediaProvider *IMediaProvider::create() { return new LinuxMediaProvider(); }
