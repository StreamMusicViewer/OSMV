#pragma once

#include "gui_backend.hpp"
#include "theme.hpp"
#include "twitch_oauth.hpp"
#include "common/types.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace osmv::gui {

enum class NavPage {
    NowPlaying = 0,
    TimeDate,
    Timers,
    SystemInfo,
    TextFiles,
    Discord,
    Twitch,
    Settings,
    Help
};

struct GuiAppState {
    AppSettings settings;
    DaemonStateSnapshot snap;
    UiContext ui;
    GuiBackend* backend = nullptr;

    NavPage current_page = NavPage::NowPlaying;
    ImTextureID album_tex = 0;
    ImTextureID demo_album_tex = 0;
    bool has_album_color = false;
    float album_hue = 260.0f;
    std::string loaded_thumb_path;
    ImTextureID logo_tex = 0;

    // OBS Overlay Studio UI state
    int obs_studio_tab = 0;       // 0=Style, 1=Layout & Content, 2=Visualizer, 3=Custom CSS, 4=Snaz
    bool obs_preview_demo = false;
    int obs_preview_backdrop = 0; // 0=Checker, 1=Dark, 2=Light
    std::string obs_new_preset_name;
    std::string obs_studio_feedback;
    double obs_studio_feedback_until = 0.0;

    bool settings_dirty = false;
    double dirty_since = 0.0;
    double saved_until = 0.0;
    bool theme_changed = false;
    bool quit_daemon_requested = false;
    bool close_gui_requested = false;

    // Timers interpolation
    std::chrono::steady_clock::time_point chrono_down_sync_tp{};
    int64_t chrono_down_sync_rem_ms = 0;
    std::chrono::steady_clock::time_point chrono_up_sync_tp{};
    int64_t chrono_up_sync_el_ms = 0;

    // Textline multiline editor cache
    std::string textline_buf;
    bool textline_buf_init = false;

    // Twitch UI helpers
    bool show_twitch_token = false;
    std::string twitch_title_input;
    std::string twitch_game_search;
    std::string twitch_selected_game_id;
    std::string twitch_selected_game_name;
    std::vector<TwitchCustomGame> twitch_search_results;
    std::string twitch_feedback;
    bool twitch_feedback_ok = true;
    bool twitch_title_synced = false;
    std::unique_ptr<TwitchOAuthListener> twitch_oauth;
    std::string twitch_oauth_msg;
    bool twitch_oauth_ok = true;

    void mark_changed_immediate();
    void mark_changed_debounced();
};

void fetch_daemon_state(GuiAppState& st);

void render_view_now_playing(GuiAppState& st);
void render_obs_widget_preview(GuiAppState& st);
bool render_css_code_editor(UiContext& ui, const char* id, std::string& css, const char* lang);
void render_view_time_date(GuiAppState& st);
void render_view_timers(GuiAppState& st);
void render_view_sysinfo(GuiAppState& st);
void render_view_text_files(GuiAppState& st);
void render_view_discord(GuiAppState& st);
void render_view_twitch(GuiAppState& st);
void render_view_settings(GuiAppState& st);
void render_view_help(GuiAppState& st);

} // namespace osmv::gui

