#include "gui_main.hpp"
#include "gui_backend.hpp"
#include "locale.hpp"
#include "theme.hpp"
#include "views.hpp"

#include "common/embedded_logo.hpp"
#include "common/ipc.hpp"
#include "common/os_utils.hpp"
#include "common/settings.hpp"
#include "common/version.hpp"

#include "IconsFontAwesome6.h"
#include <nlohmann/json.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

namespace osmv::gui {

void fetch_daemon_state(GuiAppState& st) {
    std::string resp;
    if (!ipc::send_request(R"({"cmd":"get_state"})", resp, 250)) return;
    try {
        auto j = nlohmann::json::parse(resp);
        if (j.value("ok", false) && j.contains("state")) {
            auto now = std::chrono::steady_clock::now();
            cfg::snapshot_from_json_string(j["state"].dump(), st.snap);
            st.chrono_down_sync_tp = now;
            st.chrono_down_sync_rem_ms = st.snap.chrono_down_remaining_ms;
            st.chrono_up_sync_tp = now;
            st.chrono_up_sync_el_ms = st.snap.chrono_up_elapsed_ms;
        }
    } catch (...) {
    }
}

namespace {
void save_and_notify_daemon(GuiAppState& st) {
    cfg::save_settings(st.settings);
    std::string resp;
    ipc::send_request(R"({"cmd":"reload_settings"})", resp, 300);
    st.settings_dirty = false;
    st.saved_until = ImGui::GetTime() + 1.8;
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

bool compute_dominant_hue(const uint8_t* rgba, int w, int h, float& out_hue) {
    if (!rgba || w <= 0 || h <= 0) return false;
    struct Bucket { uint64_t r = 0, g = 0, b = 0, count = 0; };
    std::array<Bucket, 36> buckets{};
    const int step_x = std::max(1, w / 80);
    const int step_y = std::max(1, h / 80);

    for (int y = 0; y < h; y += step_y) {
        for (int x = 0; x < w; x += step_x) {
            const uint8_t* p = rgba + (static_cast<size_t>(y) * w + x) * 4;
            if (p[3] < 128) continue;
            float r = p[0] / 255.0f;
            float g = p[1] / 255.0f;
            float b = p[2] / 255.0f;
            float mx = std::max({r, g, b});
            float mn = std::min({r, g, b});
            float l = (mx + mn) * 0.5f;
            if (mx == mn || l < 0.1f || l > 0.9f) continue;
            float d = mx - mn;
            float s = (l > 0.5f) ? d / (2.0f - mx - mn) : d / (mx + mn);
            if (s < 0.15f) continue;

            float hue = 0.0f;
            if (mx == r) hue = ((g - b) / d + (g < b ? 6.0f : 0.0f)) / 6.0f;
            else if (mx == g) hue = ((b - r) / d + 2.0f) / 6.0f;
            else hue = ((r - g) / d + 4.0f) / 6.0f;
            int idx = std::clamp(static_cast<int>(hue * 36.0f), 0, 35);
            buckets[idx].r += p[0];
            buckets[idx].g += p[1];
            buckets[idx].b += p[2];
            buckets[idx].count++;
        }
    }

    const Bucket* best = &buckets[0];
    for (const auto& bk : buckets) {
        if (bk.count > best->count) best = &bk;
    }
    if (best->count == 0) return false;

    float r = static_cast<float>(best->r / best->count) / 255.0f;
    float g = static_cast<float>(best->g / best->count) / 255.0f;
    float b = static_cast<float>(best->b / best->count) / 255.0f;
    float mx = std::max({r, g, b});
    float mn = std::min({r, g, b});
    if (mx == mn) {
        out_hue = 260.0f;
        return true;
    }
    float d = mx - mn;
    float hue = 0.0f;
    if (mx == r) hue = ((g - b) / d + (g < b ? 6.0f : 0.0f)) / 6.0f;
    else if (mx == g) hue = ((b - r) / d + 2.0f) / 6.0f;
    else hue = ((r - g) / d + 4.0f) / 6.0f;
    out_hue = hue * 360.0f;
    return true;
}

ImTextureID make_demo_album_texture(GuiBackend& backend) {
    constexpr int S = 96;
    std::vector<uint8_t> rgba(static_cast<size_t>(S) * S * 4);
    for (int y = 0; y < S; ++y) {
        for (int x = 0; x < S; ++x) {
            float u = static_cast<float>(x) / (S - 1);
            float v = static_cast<float>(y) / (S - 1);
            float t = (u + v) * 0.5f;
            float r = (t < 0.5f) ? (255.0f * (1.0f - t) + 124.0f * t) : (124.0f * (1.0f - (t - 0.5f) * 2.0f));
            float g = 60.0f + 150.0f * v;
            float b = 140.0f + 115.0f * u;

            float dx = u - 0.5f;
            float dy = v - 0.5f;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (std::abs(dist - 0.28f) < 0.025f) {
                r = std::min(255.0f, r + 55.0f);
                g = std::min(255.0f, g + 55.0f);
                b = std::min(255.0f, b + 55.0f);
            } else if (dist < 0.09f) {
                r = 18.0f;
                g = 20.0f;
                b = 30.0f;
            }
            size_t i = (static_cast<size_t>(y) * S + x) * 4;
            rgba[i + 0] = static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f));
            rgba[i + 1] = static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f));
            rgba[i + 2] = static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f));
            rgba[i + 3] = 255;
        }
    }
    return backend.create_rgba_texture(S, S, rgba.data());
}

ImTextureID load_texture_from_png_mem(GuiBackend& backend, const uint8_t* data, size_t len) {
    int w = 0, h = 0, ch = 0;
    uint8_t* rgba = stbi_load_from_memory(data, static_cast<int>(len), &w, &h, &ch, 4);
    if (!rgba) return 0;
    ImTextureID tex = backend.create_rgba_texture(w, h, rgba);
    stbi_image_free(rgba);
    return tex;
}

void sync_album_texture(GuiAppState& st) {
    if (st.snap.thumbnail_file_path == st.loaded_thumb_path) return;
    if (st.album_tex) {
        st.backend->destroy_texture(st.album_tex);
        st.album_tex = 0;
    }
    st.has_album_color = false;
    st.album_hue = 260.0f;
    st.loaded_thumb_path = st.snap.thumbnail_file_path;
    if (st.loaded_thumb_path.empty()) return;

    std::ifstream f(os::path_from_utf8(st.loaded_thumb_path), std::ios::binary);
    if (!f) return;
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (bytes.empty()) return;

    int w = 0, h = 0, ch = 0;
    uint8_t* rgba = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &ch, 4);
    if (!rgba) return;
    st.has_album_color = compute_dominant_hue(rgba, w, h, st.album_hue);
    st.album_tex = st.backend->create_rgba_texture(w, h, rgba);
    stbi_image_free(rgba);
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
                std::snprintf(rbuf, sizeof(rbuf), "v" OSMV_VERSION_STR "  |  %.1f MB", st.snap.daemon_ram_kb / 1024.0);
                ImGui::TextColored(c.text_dim, "%s", rbuf);
            } else {
                ImGui::TextColored(c.text_dim, "v" OSMV_VERSION_STR " C++");
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
    st.demo_album_tex = make_demo_album_texture(*backend);

    fetch_daemon_state(st);
    sync_album_texture(st);

    auto last_poll = std::chrono::steady_clock::now();

    while (!backend->should_close() && !st.close_gui_requested && !st.quit_daemon_requested) {
        bool timers_running = st.snap.chrono_down_running || st.snap.chrono_up_running;
        double wait_sec = (st.ui.animating || backend->has_active_interaction() || timers_running) ? 0.016 : 0.25;
        backend->wait_events(wait_sec);

        auto now = std::chrono::steady_clock::now();
        auto poll_interval = timers_running ? std::chrono::milliseconds(250) : std::chrono::milliseconds(500);
        if (now - last_poll >= poll_interval) {
            last_poll = now;
            fetch_daemon_state(st);
            sync_album_texture(st);
        }

        if (st.twitch_oauth) {
            std::string tok, err;
            if (st.twitch_oauth->poll_result(tok, err)) {
                if (!tok.empty()) {
                    st.settings.twitch.oauth_token = tok;
                    st.twitch_oauth_ok = true;
                    st.twitch_oauth_msg = tr(st.settings.general.language, "tw_oauth_ok");
                    st.mark_changed_immediate();
                } else {
                    st.twitch_oauth_ok = false;
                    st.twitch_oauth_msg = err.empty() ? tr(st.settings.general.language, "tw_oauth_err") : err;
                }
            }
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
    if (st.demo_album_tex) backend->destroy_texture(st.demo_album_tex);
    if (st.logo_tex) backend->destroy_texture(st.logo_tex);
    backend->shutdown();
    return 0;
}

} // namespace osmv::gui

