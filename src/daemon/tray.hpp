#pragma once

#include <functional>
#include <memory>
#include <string>

namespace osmv::daemon {

struct TrayCallbacks {
    std::function<void()> on_show_gui;
    std::function<void()> on_quit;
};

class TrayController {
public:
    virtual ~TrayController() = default;
    virtual bool init(const TrayCallbacks& callbacks) = 0;
    virtual void set_language(const std::string& lang) = 0;
    virtual void run_event_loop() = 0;
    virtual void request_quit() = 0;
};

std::unique_ptr<TrayController> create_tray_controller();

} // namespace osmv::daemon

