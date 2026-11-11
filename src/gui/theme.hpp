#pragma once

#include "imgui.h"
#include <string>

namespace osmv::gui {

struct ThemeColors {
    bool is_dark = true;
    ImVec4 bg_window;
    ImVec4 bg_sidebar;
    ImVec4 bg_card;
    ImVec4 bg_input;
    ImVec4 bg_hover;
    ImVec4 border;
    ImVec4 text_main;
    ImVec4 text_sub;
    ImVec4 text_dim;
    ImVec4 accent;
    ImVec4 accent_hover;
    ImVec4 accent_soft;
    ImVec4 success;
    ImVec4 warning;
    ImVec4 danger;
};

struct UiContext {
    ThemeColors colors;
    bool animating = false;
    double copied_until = 0.0;
    std::string copied_id;
    ImFont* font_regular = nullptr;
    ImFont* font_title = nullptr;
    ImFont* font_small = nullptr;
    ImFont* font_code = nullptr;
};

void load_fonts(float scale, UiContext& ui);
ThemeColors make_theme_colors(const std::string& theme_name, const std::string& accent_name);
void apply_imgui_style(const ThemeColors& c);

// Custom clear & friendly UI widgets
void page_header(UiContext& ui, const char* icon, const char* title, const char* subtitle);
bool toggle_switch(UiContext& ui, const char* id, const char* label, bool* value);
bool begin_card(UiContext& ui, const char* id, float height = 0.0f);
void end_card();
void section_title(UiContext& ui, const char* title, const char* hint = nullptr);
void status_pill(const ThemeColors& c, const char* text, ImVec4 dot_color);
bool token_chip(UiContext& ui, const char* token, std::string& target_str, const char* tooltip = nullptr);
void copy_path_row(UiContext& ui, const char* id, const char* label, const std::string& path_str, const std::string& lang);
void live_preview_box(UiContext& ui, const char* label, const std::string& text);
bool accent_button(const ThemeColors& c, const char* label, const ImVec2& size = ImVec2(0, 0));
bool subtle_button(const ThemeColors& c, const char* label, const ImVec2& size = ImVec2(0, 0));
bool danger_button(const ThemeColors& c, const char* label, const ImVec2& size = ImVec2(0, 0));

} // namespace osmv::gui

