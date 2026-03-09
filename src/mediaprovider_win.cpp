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
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
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
    m_cachedInfo.status = "closed";
    m_worker = std::thread([this]() {
      try {
        winrt::init_apartment();
      } catch (...) {}

      GlobalSystemMediaTransportControlsSessionManager manager{nullptr};
      try {
        manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
      } catch (...) {}

      while (m_running) {
        SongInfo info;
        if (!manager) {
          info.status = "closed";
        } else {
          try {
            auto session = manager.GetCurrentSession();
            if (!session) {
              info.status = "closed";
            } else {
              auto playback = session.GetPlaybackInfo();
              if (playback) {
                using Status = GlobalSystemMediaTransportControlsSessionPlaybackStatus;
                switch (playback.PlaybackStatus()) {
                case Status::Closed: info.status = "closed"; break;
                case Status::Opened: info.status = "opened"; break;
                case Status::Changing: info.status = "changing"; break;
                case Status::Stopped: info.status = "stopped"; break;
                case Status::Playing: info.status = "playing"; info.isPlaying = true; break;
                case Status::Paused: info.status = "paused"; break;
                default: info.status = "unknown"; break;
                }
              }

              auto props = session.TryGetMediaPropertiesAsync().get();
              if (props) {
                info.title = QString::fromStdWString(props.Title().c_str());
                info.artist = QString::fromStdWString(props.Artist().c_str());
                info.album = QString::fromStdWString(props.AlbumTitle().c_str());
                if (info.title.isEmpty()) info.title = "Unknown Title";
                if (info.artist.isEmpty()) info.artist = "Unknown Artist";
                info.thumbnailB64 = streamToBase64(props.Thumbnail());
              }
            }
          } catch (...) {
            info.status = "closed";
          }
        }

        {
          std::lock_guard<std::mutex> lock(m_mutex);
          m_cachedInfo = info;
        }

        for (int i = 0; i < 10 && m_running; ++i) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      }
    });
  }

  ~WindowsMediaProvider() override {
    m_running = false;
    if (m_worker.joinable()) {
      m_worker.join();
    }
  }

  SongInfo currentSong() override {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cachedInfo;
  }

private:
  std::mutex m_mutex;
  SongInfo m_cachedInfo;
  std::atomic<bool> m_running{true};
  std::thread m_worker;
};

// ── Factory
// ───────────────────────────────────────────────────────────────────
IMediaProvider *IMediaProvider::create() { return new WindowsMediaProvider(); }
