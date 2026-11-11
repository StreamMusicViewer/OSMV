#pragma once

#include "common/types.hpp"
#include <memory>
#include <string>

namespace osmv::daemon {

struct PolledMedia {
    SongInfo song;
    std::string player_name;
};

class MediaProvider {
public:
    virtual ~MediaProvider() = default;
    virtual PolledMedia current_song() = 0;
};

std::unique_ptr<MediaProvider> create_media_provider();

} // namespace osmv::daemon

