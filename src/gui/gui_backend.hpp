#pragma once

#include "imgui.h"
#include <cstdint>
#include <memory>

namespace osmv::gui {

class GuiBackend {
public:
    virtual ~GuiBackend() = default;

    virtual bool init(const char* title, int width, int height, bool dark_titlebar) = 0;
    virtual void shutdown() = 0;

    virtual bool should_close() const = 0;
    virtual void wait_events(double timeout_sec) = 0;
    virtual bool begin_frame() = 0;
    virtual void end_frame(float clear_r, float clear_g, float clear_b) = 0;

    virtual void set_dark_titlebar(bool dark) = 0;
    virtual float content_scale() const = 0;
    virtual bool has_active_interaction() const = 0;

    virtual ImTextureID create_rgba_texture(int width, int height, const uint8_t* rgba) = 0;
    virtual void destroy_texture(ImTextureID tex) = 0;
};

std::unique_ptr<GuiBackend> create_gui_backend();

} // namespace osmv::gui

