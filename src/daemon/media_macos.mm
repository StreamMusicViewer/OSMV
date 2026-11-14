#include "media_provider.hpp"

#import <Cocoa/Cocoa.h>
#include <sstream>
#include <string>
#include <vector>

namespace osmv::daemon {

namespace {

static NSString* const kAppleScriptSource =
    @"if application \"Spotify\" is running then\n"
    @"  tell application \"Spotify\"\n"
    @"    if player state is playing or player state is paused then\n"
    @"      set pstate to \"paused\"\n"
    @"      if player state is playing then set pstate to \"playing\"\n"
    @"      set art to \"\"\n"
    @"      try\n"
    @"        set art to artwork url of current track\n"
    @"      end try\n"
    @"      return \"Spotify\\n\" & pstate & \"\\n\" & (name of current track) & \"\\n\" & (artist of current track) & \"\\n\" & (album of current track) & \"\\n\" & art\n"
    @"    end if\n"
    @"  end tell\n"
    @"end if\n"
    @"if application \"Music\" is running then\n"
    @"  tell application \"Music\"\n"
    @"    if player state is playing or player state is paused then\n"
    @"      set pstate to \"paused\"\n"
    @"      if player state is playing then set pstate to \"playing\"\n"
    @"      return \"Music\\n\" & pstate & \"\\n\" & (name of current track) & \"\\n\" & (artist of current track) & \"\\n\" & (album of current track) & \"\\n\"\n"
    @"    end if\n"
    @"  end tell\n"
    @"end if\n"
    @"return \"\"";

class MacOSMediaProvider final : public MediaProvider {
public:
    MacOSMediaProvider() {
        @autoreleasepool {
            script_ = [[NSAppleScript alloc] initWithSource:kAppleScriptSource];
            [script_ compileAndReturnError:nil];
        }
    }

    ~MacOSMediaProvider() override {
        @autoreleasepool {
            if (script_) {
                [script_ release];
                script_ = nil;
            }
        }
    }

    PolledMedia current_song() override {
        PolledMedia out;
        out.song.status = "closed";

        @autoreleasepool {
            if (!script_) return out;

            NSDictionary* error_dict = nil;
            NSAppleEventDescriptor* desc = [script_ executeAndReturnError:&error_dict];
            if (!desc || error_dict) {
                return out;
            }

            NSString* str = [desc stringValue];
            if (!str || [str length] == 0) {
                return out;
            }

            std::string res = [str UTF8String];
            std::stringstream ss(res);
            std::string line;
            std::vector<std::string> lines;
            while (std::getline(ss, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                lines.push_back(line);
            }

            if (lines.size() >= 5) {
                out.player_name = lines[0];
                out.song.status = lines[1];
                out.song.is_playing = (lines[1] == "playing");
                out.song.title = lines[2];
                out.song.artist = lines[3];
                out.song.album = lines[4];
                if (lines.size() >= 6) {
                    out.song.art_url = lines[5];
                }
            }
        }
        return out;
    }

private:
    NSAppleScript* script_ = nil;
};

} // namespace

std::unique_ptr<MediaProvider> create_media_provider() {
    return std::make_unique<MacOSMediaProvider>();
}

} // namespace osmv::daemon
