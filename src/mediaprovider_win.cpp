/// mediaprovider_win.cpp
/// Windows implementation of IMediaProvider using WinRT
/// GlobalSystemMediaTransportControlsSessionManager (SMTC).
///
/// Requires: Windows 10 SDK 10.0.19041+, compiled with /std:c++20
/// Only compiled on Windows (CMakeLists.txt if(WIN32) guard).

#include "mediaprovider.h"

// WinRT / C++/WinRT headers (Windows SDK)
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>

#include <QByteArray>

using namespace winrt;
using namespace winrt::Windows::Media::Control;
using namespace winrt::Windows::Storage::Streams;

// ── Helper: download IRandomAccessStreamReference → base64
// ────────────────────
static QString streamToBase64(IRandomAccessStreamReference streamRef) {
  if (!streamRef)
    return {};
  try {
    auto stream = streamRef.OpenReadAsync().get();
    uint64_t size = stream.Size();
    if (size == 0)
      return {};

    DataReader reader(stream);
    reader.LoadAsync(static_cast<uint32_t>(size)).get();
    std::vector<uint8_t> buf(size);
    reader.ReadBytes(buf);

    QByteArray data(reinterpret_cast<const char *>(buf.data()),
                    static_cast<int>(buf.size()));
    return QString::fromLatin1(data.toBase64());
  } catch (...) {
    return {};
  }
}

// ── Windows implementation
// ────────────────────────────────────────────────────
class WindowsMediaProvider : public IMediaProvider {
public:
  WindowsMediaProvider() {
    winrt::init_apartment();
    m_manager =
        GlobalSystemMediaTransportControlsSessionManager ::RequestAsync().get();
  }

  SongInfo currentSong() override {
    SongInfo info;
    if (!m_manager) {
      info.status = "closed";
      return info;
    }

    auto session = m_manager.GetCurrentSession();
    if (!session) {
      info.status = "closed";
      return info;
    }

    // ── Playback status ────────────────────────────────────────────────
    auto playback = session.GetPlaybackInfo();
    if (playback) {
      using Status = GlobalSystemMediaTransportControlsSessionPlaybackStatus;
      switch (playback.PlaybackStatus()) {
      case Status::Closed:
        info.status = "closed";
        break;
      case Status::Opened:
        info.status = "opened";
        break;
      case Status::Changing:
        info.status = "changing";
        break;
      case Status::Stopped:
        info.status = "stopped";
        break;
      case Status::Playing:
        info.status = "playing";
        info.isPlaying = true;
        break;
      case Status::Paused:
        info.status = "paused";
        break;
      default:
        info.status = "unknown";
        break;
      }
    }

    // ── Metadata ──────────────────────────────────────────────────────
    try {
      auto props = session.TryGetMediaPropertiesAsync().get();
      if (!props)
        return info;

      info.title = QString::fromStdWString(props.Title().c_str());
      info.artist = QString::fromStdWString(props.Artist().c_str());
      info.album = QString::fromStdWString(props.AlbumTitle().c_str());

      if (info.title.isEmpty())
        info.title = "Unknown Title";
      if (info.artist.isEmpty())
        info.artist = "Unknown Artist";

      // ── Thumbnail ─────────────────────────────────────────────────
      info.thumbnailB64 = streamToBase64(props.Thumbnail());
    } catch (...) {
    }

    return info;
  }

private:
  GlobalSystemMediaTransportControlsSessionManager m_manager{nullptr};
};

// ── Factory
// ───────────────────────────────────────────────────────────────────
IMediaProvider *IMediaProvider::create() { return new WindowsMediaProvider(); }
