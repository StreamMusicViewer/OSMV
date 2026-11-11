#include "gui_main.hpp"
#include "gui_backend.hpp"
#include "locale.hpp"
#include "theme.hpp"
#include "views.hpp"

#include "common/embedded_logo.hpp"
#include "common/ipc.hpp"
#include "common/os_utils.hpp"
#include "common/settings.hpp"

#include "IconsFontAwesome6.h"
#include <nlohmann/json.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

namespace osmv::gui {

namespace {
void save_and_notify_daemon(GuiAppState& st) {
    cfg::save_settings(st.settings);
    std::string resp;
    ipc::send_request(R"({"cmd":"reload_settings"})", resp, 300);
    st.settings_dirty = false;
    st.saved_until = ImGui::GetTime() + 1.8;
}

void fetch_daemon_state(GuiAppState& st) {
    std::string resp;
    if (!ipc::send_request(R"({"cmd":"get_state"})", resp, 250)) return;
    try {
        auto j = nlohmann::json::parse(resp);
        if (j.value("ok", false) && j.contains("state")) {
            cfg::snapshot_from_json_string(j["state"].dump(), st.snap);
        }
    } catch (...) {
    }
}

void ensure_daemon_running() {
    std::string resp;
    if (ipc::send_request(R"({"cmd":"get_state"})", resp, 150)) return;

#ifdef _WIN32
    os::spawn_detached(os::exe_path(), {"--silent"});
#else
    auto daemon_bin = os::exe_dir() / "osmv";
    std::error_code ec;
    if (std::filesystem::exists(daemon_bin, ec)) {
        os::spawn_detached(daemon_bin, {"--silent"});
    } else {
        os::spawn_detached(os::exe_path(), {"--silent"});
    }
#endif
    for (int i = 0; i < 12; ++i) {
        os::sleep_ms(50);
        if (ipc::send_request(R"({"cmd":"get_state"})", resp, 100)) break;
    }
}

ImTextureID load_texture_from_png_mem(GuiBackend& backend, const uint8_t* data, size_t len) {
    int w = 0, h = 0, ch = 0;
    uint8_t* rgba = stbi_load_from_memory(data, static_cast<int>(len), &w, &h, &ch, 4);
    if (!rgba) return 0;
    ImTextureID tex = backend.create_rgba_texture(w, h, rgba);
    stbi_image_free(rgba);
    return tex;
}

ImTextureID load_texture_from_file(GuiBackend& backend, const std::string& utf8_path) {
    if (utf8_path.empty()) return 0;
    std::ifstream f(os::path_from_utf8(utf8_path), std::ios::binary);
    if (!f) return 0;
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (bytes.empty()) return 0;
    return load_texture_from_png_mem(backend, bytes.data(), bytes.size());
}

void sync_album_texture(GuiAppState& st) {
    if (st.snap.thumbnail_file_path == st.loaded_thumb_path) return;
    if (st.album_tex) {
        st.backend->destroy_texture(st.album_tex);
        st.album_tex = 0;
    }
    st.loaded_thumb_path = st.snap.thumbnail_file_path;
    if (!st.loaded_thumb_path.empty()) {
        st.album_tex = load_texture_from_file(*st.backend, st.loaded_thumb_path);
    }
}

void render_sidebar(GuiAppState& st, float sidebar_w) {
    const auto& lang = st.settings.general.language;
    const auto& c = st.ui.colors;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, c.bg_sidebar);
    ImGui::PushStyleColor(ImGuiCol_Border, c.border);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 14));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);

    if (ImGui::BeginChild("##sidebar", ImVec2(sidebar_w, -1), ImGuiChildFlags_AlwaysUseWindowPadding)) {
        // Brand Header
        if (st.logo_tex) {
            ImGui::Image(st.logo_tex, ImVec2(30, 30));
            ImGui::SameLine(0, 10);
        }
        ImGui::BeginGroup();
        {
            ImGui::PushFont(st.ui.font_title);
            ImGui::TextUnformatted("OSMV");
            ImGui::PopFont();
            ImGui::PushFont(st.ui.font_small);
            if (st.snap.daemon_ram_kb > 0) {
                char rbuf[48];
                std::snprintf(rbuf, sizeof(rbuf), "v5.0.0  |  %.1f MB", st.snap.daemon_ram_kb / 1024.0);
                ImGui::TextColored(c.text_dim, "%s", rbuf);
            } else {
                ImGui::TextColored(c.text_dim, "v5.0.0 C++");
            }
            ImGui::PopFont();
        }
        ImGui::EndGroup();

        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 8));

        auto nav_item = [&](NavPage page, const char* icon, const char* key, bool active_dot) {
            bool selected = (st.current_page == page);
            ImGui::PushID(static_cast<int>(page));

            ImVec4 bg = selected ? c.accent_soft : ImVec4(0, 0, 0, 0);
            ImVec4 txt = selected ? c.accent : c.text_sub;
            ImGui::PushStyleColor(ImGuiCol_Button, bg);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, selected ? c.accent_soft : c.bg_hover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, c.accent_soft);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Text, txt);
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

            std::string lbl = std::string("  ") + icon + "   " + tr(lang, key);
            ImVec2 p = ImGui::GetCursorScreenPos();
            float bw = ImGui::GetContentRegionAvail().x;
            if (ImGui::Button(lbl.c_str(), ImVec2(bw, 34))) {
                st.current_page = page;
            }
            if (active_dot) {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddCircleFilled(ImVec2(p.x + bw - 12.0f, p.y + 17.0f), 3.8f,
                                    ImGui::ColorConvertFloat4ToU32(c.success));
            }
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(5);
            ImGui::PopID();
        };

        const auto& s = st.settings;
        nav_item(NavPage::NowPlaying, ICON_FA_MUSIC, "nav_now_playing", s.now_playing_enabled);
        nav_item(NavPage::TimeDate, ICON_FA_CLOCK, "nav_time_date", s.time.enabled || s.date.enabled);
        nav_item(NavPage::Timers, ICON_FA_STOPWATCH, "nav_timers", s.countdown.enabled || s.chrono_down.enabled || s.chrono_up.enabled);
        nav_item(NavPage::SystemInfo, ICON_FA_MICROCHIP, "nav_sysinfo", s.system_info.enabled);
        nav_item(NavPage::TextFiles, ICON_FA_FILE_LINES, "nav_text_files", s.textline.enabled || s.dynamic_files.enabled);
        nav_item(NavPage::Discord, ICON_FA_GAMEPAD, "nav_discord", s.discord.enabled);
        nav_item(NavPage::Twitch, ICON_FA_TV, "nav_twitch", s.twitch.enabled);

        ImGui::Dummy(ImVec2(0, 6));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 6));

        nav_item(NavPage::Settings, ICON_FA_GEAR, "nav_settings", false);
        nav_item(NavPage::Help, ICON_FA_CIRCLE_QUESTION, "nav_help", false);

        // Footer aligned to bottom
        float rem_y = ImGui::GetContentRegionAvail().y;
        float footer_h = 98.0f;
        if (rem_y > footer_h) ImGui::Dummy(ImVec2(0, rem_y - footer_h));

        if (ImGui::GetTime() < st.saved_until) {
            ImGui::PushFont(st.ui.font_small);
            ImGui::TextColored(c.success, ICON_FA_CHECK "  %s", tr(lang, "saved_ok"));
            ImGui::PopFont();
            st.ui.animating = true;
        } else {
            ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));
        }

        std::string h_lbl = std::string(ICON_FA_MOON "  ") + tr(lang, "btn_headless");
        if (subtle_button(c, h_lbl.c_str(), ImVec2(-1, 32))) {
            st.close_gui_requested = true;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            ImGui::SetTooltip("%s", tr(lang, "btn_headless_tip"));
        }

        std::string q_lbl = std::string(ICON_FA_POWER_OFF "  ") + tr(lang, "btn_quit");
        if (danger_button(c, q_lbl.c_str(), ImVec2(-1, 32))) {
            st.quit_daemon_requested = true;
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}
} // namespace

void GuiAppState::mark_changed_immediate() {
    save_and_notify_daemon(*this);
}

void GuiAppState::mark_changed_debounced() {
    settings_dirty = true;
    dirty_since = ImGui::GetTime();
}

int run_gui() {
    os::SingleInstanceGuard gui_guard;
    if (!gui_guard.try_acquire("osmv-gui-instance-v3")) {
        return 0;
    }

    ensure_daemon_running();

    GuiAppState st;
    st.settings = cfg::load_settings();

    auto backend = create_gui_backend();
    st.backend = backend.get();
    const bool is_dark = (st.settings.general.theme != "light");
    if (!backend->init("OSMV - OBS Stream Music Viewer", 900, 600, is_dark)) {
        return 1;
    }

    load_fonts(backend->content_scale(), st.ui);
    st.ui.colors = make_theme_colors(st.settings.general.theme, st.settings.general.accent);
    apply_imgui_style(st.ui.colors);
    st.logo_tex = load_texture_from_png_mem(*backend, assets::kLogoPng, assets::kLogoPngSize);

    fetch_daemon_state(st);
    sync_album_texture(st);

    auto last_poll = std::chrono::steady_clock::now();

    while (!backend->should_close() && !st.close_gui_requested && !st.quit_daemon_requested) {
        double wait_sec = (st.ui.animating || backend->has_active_interaction()) ? 0.016 : 0.25;
        backend->wait_events(wait_sec);

        auto now = std::chrono::steady_clock::now();
        if (now - last_poll >= std::chrono::milliseconds(500)) {
            last_poll = now;
            fetch_daemon_state(st);
            sync_album_texture(st);
        }

        if (st.settings_dirty && (ImGui::GetTime() - st.dirty_since) >= 0.35) {
            save_and_notify_daemon(st);
        }

        if (st.theme_changed) {
            st.ui.colors = make_theme_colors(st.settings.general.theme, st.settings.general.accent);
            apply_imgui_style(st.ui.colors);
            backend->set_dark_titlebar(st.ui.colors.is_dark);
            st.theme_changed = false;
        }

        if (!backend->begin_frame()) continue;
        st.ui.animating = false;

        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);
        ImGuiWindowFlags root_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                                      ImGuiWindowFlags_NoBringToFrontOnFocus;

        if (ImGui::Begin("##OSMV_Root", nullptr, root_flags)) {
            const float sidebar_w = 212.0f;
            render_sidebar(st, sidebar_w);

            ImGui::SameLine(0, 0);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 sep_pos = ImGui::GetCursorScreenPos();
            dl->AddLine(sep_pos, ImVec2(sep_pos.x, sep_pos.y + vp->Size.y),
                        ImGui::ColorConvertFloat4ToU32(st.ui.colors.border), 1.0f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 1.0f);

            ImGui::PushStyleColor(ImGuiCol_ChildBg, st.ui.colors.bg_window);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22, 18));
            if (ImGui::BeginChild("##content_pane", ImVec2(-1, -1), ImGuiChildFlags_AlwaysUseWindowPadding)) {
                switch (st.current_page) {
                    case NavPage::NowPlaying: render_view_now_playing(st); break;
                    case NavPage::TimeDate:   render_view_time_date(st); break;
                    case NavPage::Timers:     render_view_timers(st); break;
                    case NavPage::SystemInfo: render_view_sysinfo(st); break;
                    case NavPage::TextFiles:  render_view_text_files(st); break;
                    case NavPage::Discord:    render_view_discord(st); break;
                    case NavPage::Twitch:     render_view_twitch(st); break;
                    case NavPage::Settings:   render_view_settings(st); break;
                    case NavPage::Help:       render_view_help(st); break;
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }
        ImGui::End();

        const auto& bg = st.ui.colors.bg_window;
        backend->end_frame(bg.x, bg.y, bg.z);
    }

    if (st.settings_dirty) {
        save_and_notify_daemon(st);
    }

    if (st.quit_daemon_requested) {
        std::string dummy;
        ipc::send_request(R"({"cmd":"quit"})", dummy, 400);
    }

    if (st.album_tex) backend->destroy_texture(st.album_tex);
    if (st.logo_tex) backend->destroy_texture(st.logo_tex);
    backend->shutdown();
    return 0;
}

} // namespace osmv::gui

