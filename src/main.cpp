#include "daemon/daemon_main.hpp"
#include "common/os_utils.hpp"

#ifdef _WIN32
#include "gui/gui_main.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#endif

#include <cstring>
#include <string>
#include <vector>

namespace {
int dispatch_args(const std::vector<std::string>& args) {
    bool is_gui = false;
    bool is_silent = false;
    for (const auto& a : args) {
        if (a == "--gui") is_gui = true;
        else if (a == "--silent") is_silent = true;
    }

#ifdef _WIN32
    if (is_gui) {
        return osmv::gui::run_gui();
    }
    return osmv::daemon::run_daemon(is_silent);
#else
    if (is_gui) {
        auto gui_bin = osmv::os::exe_dir() / "osmv-gui";
        std::string gstr = gui_bin.string();
        char* const argv[] = {const_cast<char*>(gstr.c_str()), nullptr};
        execv(gstr.c_str(), argv);
        return 1;
    }
    return osmv::daemon::run_daemon(is_silent);
#endif
}
} // namespace

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    int argc = 0;
    LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::string> args;
    if (wargv) {
        for (int i = 1; i < argc; ++i) {
            args.push_back(osmv::os::wide_to_utf8(wargv[i]));
        }
        LocalFree(wargv);
    }
    return dispatch_args(args);
}
#else
int main(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    return dispatch_args(args);
}
#endif
