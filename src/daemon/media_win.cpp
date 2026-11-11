#include "media_provider.hpp"
#include "common/formatter.hpp"
#include "common/os_utils.hpp"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>

#include <vector>

namespace osmv::daemon {

namespace {
using namespace winrt::Windows::Media::Control;
using namespace winrt::Windows::Storage::Streams;

std::string stream_ref_to_base64(const IRandomAccessStreamReference& stream_ref) {
    if (!stream_ref) return {};
    try {
        auto stream = stream_ref.OpenReadAsync().get();
        if (!stream) return {};
        uint64_t size = stream.Size();
        if (size == 0 || size > 10 * 1024 * 1024) return {};

        DataReader reader(stream.GetInputStreamAt(0));
        uint32_t loaded = reader.LoadAsync(static_cast<uint32_t>(size)).get();
        if (loaded == 0) return {};

        std::vector<uint8_t> buf(loaded);
        reader.ReadBytes(buf);
        reader.Close();
        stream.Close();
        return fmt::base64_encode(buf);
    } catch (...) {
        return {};
    }
}

std::string clean_app_name(const std::string& raw_aumid) {
    if (raw_aumid.empty()) return "Windows Media";
    std::string s = raw_aumid;
    auto bang = s.find('!');
    if (bang != std::string::npos) s = s.substr(0, bang);
    auto dot = s.rfind('.');
    if (dot != std::string::npos && dot + 1 < s.size()) {
        std::string tail = s.substr(dot + 1);
        if (tail != "exe" && tail != "EXE") return tail;
    }
    return s;
}

class WindowsMediaProvider final : public MediaProvider {
public:
    WindowsMediaProvider() {
        try {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
        } catch (const winrt::hresult_error&) {
            // Apartment already initialized with another mode; WinRT calls still work.
        }
        ensure_manager();
    }

    PolledMedia current_song() override {
        PolledMedia res;
        res.song.status = "closed";

        if (!ensure_manager()) return res;

        try {
            auto session = manager_.GetCurrentSession();
            if (!session) {
                last_sig_.clear();
                last_thumb_b64_.clear();
                return res;
            }

            try {
                res.player_name = clean_app_name(os::wide_to_utf8(std::wstring(session.SourceAppUserModelId())));
            } catch (...) {
                res.player_name = "Windows Media";
            }

            auto playback = session.GetPlaybackInfo();
            if (playback) {
                switch (playback.PlaybackStatus()) {
                    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Closed:
                        res.song.status = "closed";
                        break;
                    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Opened:
                        res.song.status = "opened";
                        break;
                    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Changing:
                        res.song.status = "changing";
                        break;
                    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped:
                        res.song.status = "stopped";
                        break;
                    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing:
                        res.song.status = "playing";
                        res.song.is_playing = true;
                        break;
                    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused:
                        res.song.status = "paused";
                        break;
                    default:
                        res.song.status = "unknown";
                        break;
                }
            }

            auto props = session.TryGetMediaPropertiesAsync().get();
            if (props) {
                res.song.title = os::wide_to_utf8(std::wstring(props.Title()));
                res.song.artist = os::wide_to_utf8(std::wstring(props.Artist()));
                res.song.album = os::wide_to_utf8(std::wstring(props.AlbumTitle()));

                if (res.song.title.empty()) res.song.title = "Unknown Title";
                if (res.song.artist.empty()) res.song.artist = "Unknown Artist";

                std::string sig = res.song.title + "|" + res.song.artist + "|" + res.song.album;
                if (sig != last_sig_) {
                    last_sig_ = std::move(sig);
                    last_thumb_b64_ = stream_ref_to_base64(props.Thumbnail());
                }
                res.song.thumbnail_b64 = last_thumb_b64_;
            }
        } catch (...) {
            // Session invalidated or SMTC service restarted; refresh manager on next poll
            manager_ = nullptr;
            last_sig_.clear();
            last_thumb_b64_.clear();
        }
        return res;
    }

private:
    bool ensure_manager() {
        if (manager_) return true;
        try {
            manager_ = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
            return static_cast<bool>(manager_);
        } catch (...) {
            manager_ = nullptr;
            return false;
        }
    }

    GlobalSystemMediaTransportControlsSessionManager manager_{nullptr};
    std::string last_sig_;
    std::string last_thumb_b64_;
};
} // namespace

std::unique_ptr<MediaProvider> create_media_provider() {
    return std::make_unique<WindowsMediaProvider>();
}

} // namespace osmv::daemon

