#include "theme.hpp"
#include "locale.hpp"
#include "common/embedded_fonts.hpp"
#include "IconsFontAwesome6.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <unordered_map>

namespace osmv::gui {

namespace {
ImVec4 rgba(uint8_t r, uint8_t g, uint8_t b, float a = 1.0f) {
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

ImVec4 lerp_col(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(a.x + (b.x - a.x) * t,
                  a.y + (b.y - a.y) * t,
                  a.z + (b.z - a.z) * t,
                  a.w + (b.w - a.w) * t);
}

const ImWchar* latin_ranges() {
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement (French accents)
        0x0100, 0x017F, // Latin Extended-A
        0x2010, 0x2027, // Dashes, quotes, bullet
        0x20AC, 0x20AC, // Euro sign
        0,
    };
    return ranges;
}

const ImWchar* fa_icon_ranges() {
    static ImVector<ImWchar> ranges;
    if (ranges.empty()) {
        ImFontGlyphRangesBuilder builder;
        const char* icons[] = {
            ICON_FA_MUSIC, ICON_FA_CLOCK, ICON_FA_CALENDAR, ICON_FA_HOURGLASS,
            ICON_FA_STOPWATCH, ICON_FA_MICROCHIP, ICON_FA_ALIGN_LEFT, ICON_FA_GLOBE,
            ICON_FA_GAMEPAD, ICON_FA_TV, ICON_FA_GEAR, ICON_FA_CIRCLE_QUESTION,
            ICON_FA_MOON, ICON_FA_SUN, ICON_FA_POWER_OFF, ICON_FA_COPY,
            ICON_FA_CHECK, ICON_FA_PLAY, ICON_FA_PAUSE, ICON_FA_ROTATE_LEFT,
            ICON_FA_TRASH, ICON_FA_FOLDER_OPEN, ICON_FA_EYE, ICON_FA_EYE_SLASH,
            ICON_FA_ARROW_UP, ICON_FA_ARROW_DOWN, ICON_FA_CIRCLE_INFO, ICON_FA_BOLT,
            ICON_FA_LINK, ICON_FA_KEY, ICON_FA_MAGNIFYING_GLASS, ICON_FA_FORWARD_STEP,
            ICON_FA_FLOPPY_DISK, ICON_FA_PALETTE, ICON_FA_LANGUAGE, ICON_FA_MEMORY,
            ICON_FA_NETWORK_WIRED, ICON_FA_COMPACT_DISC, ICON_FA_SHUFFLE, ICON_FA_SLIDERS,
            ICON_FA_DESKTOP, ICON_FA_FILE_LINES, ICON_FA_HEADPHONES, ICON_FA_WAVE_SQUARE,
            ICON_FA_ARROW_UP_RIGHT_FROM_SQUARE, ICON_FA_CODE, ICON_FA_BOOKMARK,
            ICON_FA_LAYER_GROUP, ICON_FA_WAND_MAGIC_SPARKLES
        };
        for (const char* ic : icons) builder.AddText(ic);
        builder.BuildRanges(&ranges);
    }
    return ranges.Data;
}
} // namespace

void load_fonts(float scale, UiContext& ui) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    const float base_px = std::round(15.5f * scale);
    const float title_px = std::round(21.0f * scale);
    const float small_px = std::round(13.0f * scale);

    auto add_merged = [&](float px, float icon_px) -> ImFont* {
        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false;
        cfg.OversampleH = 2;
        cfg.OversampleV = 1;
        cfg.PixelSnapH = true;
        ImFont* f = io.Fonts->AddFontFromMemoryTTF(
            const_cast<unsigned char*>(assets::kInterFontTtf),
            static_cast<int>(assets::kInterFontTtfSize),
            px, &cfg, latin_ranges());

        ImFontConfig icfg;
        icfg.FontDataOwnedByAtlas = false;
        icfg.MergeMode = true;
        icfg.PixelSnapH = true;
        icfg.GlyphMinAdvanceX = icon_px;
        io.Fonts->AddFontFromMemoryTTF(
            const_cast<unsigned char*>(assets::kFaSolidTtf),
            static_cast<int>(assets::kFaSolidTtfSize),
            icon_px, &icfg, fa_icon_ranges());
        return f;
    };

    ui.font_regular = add_merged(base_px, base_px * 0.92f);
    ui.font_title = add_merged(title_px, title_px * 0.90f);
    ui.font_small = add_merged(small_px, small_px * 0.92f);

    // Load monospace code font from OS font directory with built-in ProggyClean fallback
    const float code_px = std::round(14.5f * scale);
    ui.font_code = nullptr;
    static constexpr const char* kMonoFontPaths[] = {
#ifdef _WIN32
        "C:\\Windows\\Fonts\\CascadiaMono.ttf",
        "C:\\Windows\\Fonts\\consola.ttf",
        "C:\\Windows\\Fonts\\lucon.ttf"
#elif defined(__APPLE__)
        "/System/Library/Fonts/Monaco.ttf",
        "/System/Library/Fonts/SFNSMono.ttf",
        "/Library/Fonts/Courier New.ttf"
#else
        "/usr/share/fonts/TTF/JetBrainsMono-Regular.ttf",
        "/usr/share/fonts/truetype/jetbrains-mono/JetBrainsMono-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        "/usr/share/fonts/liberation-mono/LiberationMono-Regular.ttf"
#endif
    };
    std::error_code ec;
    for (const char* path : kMonoFontPaths) {
        if (std::filesystem::exists(path, ec)) {
            ImFontConfig mcfg;
            mcfg.OversampleH = 2;
            mcfg.OversampleV = 1;
            mcfg.PixelSnapH = true;
            ui.font_code = io.Fonts->AddFontFromFileTTF(path, code_px, &mcfg, latin_ranges());
            if (ui.font_code) break;
        }
    }
    if (!ui.font_code) {
        ImFontConfig dcfg;
        dcfg.SizePixels = std::round(13.0f * scale);
        ui.font_code = io.Fonts->AddFontDefault(&dcfg);
    }

    io.FontDefault = ui.font_regular;
    io.Fonts->Build();
}

ThemeColors make_theme_colors(const std::string& theme_name, const std::string& accent_name) {
    ThemeColors c;
    c.is_dark = (theme_name != "light");

    if (accent_name == "emerald") {
        c.accent = rgba(16, 185, 129);
        c.accent_hover = rgba(5, 150, 105);
    } else if (accent_name == "cyan") {
        c.accent = rgba(6, 182, 212);
        c.accent_hover = rgba(8, 145, 178);
    } else if (accent_name == "rose") {
        c.accent = rgba(244, 63, 94);
        c.accent_hover = rgba(225, 29, 72);
    } else if (accent_name == "amber") {
        c.accent = rgba(245, 158, 11);
        c.accent_hover = rgba(217, 119, 6);
    } else {
        // Default Discord Blurple / Indigo
        c.accent = rgba(88, 101, 242);
        c.accent_hover = rgba(71, 82, 196);
    }
    c.accent_soft = ImVec4(c.accent.x, c.accent.y, c.accent.z, c.is_dark ? 0.16f : 0.12f);

    c.success = rgba(34, 197, 94);
    c.warning = rgba(245, 158, 11);
    c.danger = rgba(239, 68, 68);

    if (c.is_dark) {
        c.bg_window  = rgba(14, 17, 23);
        c.bg_sidebar = rgba(18, 22, 31);
        c.bg_card    = rgba(23, 28, 39);
        c.bg_input   = rgba(13, 16, 22);
        c.bg_hover   = rgba(33, 40, 56);
        c.border     = rgba(41, 49, 68);
        c.text_main  = rgba(241, 245, 249);
        c.text_sub   = rgba(148, 163, 184);
        c.text_dim   = rgba(100, 116, 139);
    } else {
        c.bg_window  = rgba(243, 246, 251);
        c.bg_sidebar = rgba(255, 255, 255);
        c.bg_card    = rgba(255, 255, 255);
        c.bg_input   = rgba(248, 250, 252);
        c.bg_hover   = rgba(236, 241, 249);
        c.border     = rgba(218, 226, 239);
        c.text_main  = rgba(15, 23, 42);
        c.text_sub   = rgba(71, 85, 105);
        c.text_dim   = rgba(148, 163, 184);
    }
    return c;
}

void apply_imgui_style(const ThemeColors& c) {
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowPadding = ImVec2(0, 0);
    st.FramePadding = ImVec2(10, 7);
    st.ItemSpacing = ImVec2(10, 8);
    st.ItemInnerSpacing = ImVec2(6, 6);
    st.ScrollbarSize = 10.0f;
    st.WindowRounding = 0.0f;
    st.ChildRounding = 12.0f;
    st.FrameRounding = 8.0f;
    st.PopupRounding = 8.0f;
    st.ScrollbarRounding = 8.0f;
    st.GrabRounding = 8.0f;
    st.FrameBorderSize = 1.0f;
    st.ChildBorderSize = 1.0f;
    st.WindowBorderSize = 0.0f;

    auto* col = st.Colors;
    col[ImGuiCol_Text] = c.text_main;
    col[ImGuiCol_TextDisabled] = c.text_dim;
    col[ImGuiCol_WindowBg] = c.bg_window;
    col[ImGuiCol_ChildBg] = c.bg_card;
    col[ImGuiCol_PopupBg] = c.bg_card;
    col[ImGuiCol_Border] = c.border;
    col[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    col[ImGuiCol_FrameBg] = c.bg_input;
    col[ImGuiCol_FrameBgHovered] = c.bg_hover;
    col[ImGuiCol_FrameBgActive] = c.bg_hover;
    col[ImGuiCol_Button] = c.bg_hover;
    col[ImGuiCol_ButtonHovered] = ImVec4(c.accent.x, c.accent.y, c.accent.z, 0.25f);
    col[ImGuiCol_ButtonActive] = c.accent;
    col[ImGuiCol_Header] = c.accent_soft;
    col[ImGuiCol_HeaderHovered] = ImVec4(c.accent.x, c.accent.y, c.accent.z, 0.22f);
    col[ImGuiCol_HeaderActive] = ImVec4(c.accent.x, c.accent.y, c.accent.z, 0.30f);
    col[ImGuiCol_CheckMark] = c.accent;
    col[ImGuiCol_SliderGrab] = c.accent;
    col[ImGuiCol_SliderGrabActive] = c.accent_hover;
    col[ImGuiCol_Separator] = c.border;
    col[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    col[ImGuiCol_ScrollbarGrab] = c.border;
    col[ImGuiCol_ScrollbarGrabHovered] = c.text_dim;
    col[ImGuiCol_ScrollbarGrabActive] = c.accent;
}

void page_header(UiContext& ui, const char* icon, const char* title, const char* subtitle) {
    ImGui::PushFont(ui.font_title);
    ImGui::TextColored(ui.colors.accent, "%s", icon);
    ImGui::SameLine(0, 10);
    ImGui::TextUnformatted(title);
    ImGui::PopFont();

    if (subtitle && subtitle[0]) {
        ImGui::PushFont(ui.font_small);
        ImGui::PushStyleColor(ImGuiCol_Text, ui.colors.text_sub);
        ImGui::TextWrapped("%s", subtitle);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }
    ImGui::Dummy(ImVec2(0, 6));
}

bool toggle_switch(UiContext& ui, const char* id, const char* label, bool* value) {
    static std::unordered_map<ImGuiID, float> s_anim;
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems) return false;

    const float h = 22.0f;
    const float w = 40.0f;
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImGuiID gid = win->GetID(id);

    ImGui::InvisibleButton(id, ImVec2(w, h));
    bool clicked = ImGui::IsItemClicked();
    bool hovered = ImGui::IsItemHovered();
    if (clicked) {
        *value = !(*value);
    }

    float& t = s_anim[gid];
    float target = *value ? 1.0f : 0.0f;
    if (s_anim.find(gid) == s_anim.end()) t = target;
    float dt = std::min(ImGui::GetIO().DeltaTime, 0.05f);
    if (std::abs(t - target) > 0.005f) {
        t += (target - t) * std::min(1.0f, dt * 16.0f);
        ui.animating = true;
    } else {
        t = target;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec4 track_off = hovered ? ui.colors.bg_hover : ui.colors.bg_input;
    ImVec4 track_col = lerp_col(track_off, ui.colors.accent, t);
    ImVec4 border_col = lerp_col(ui.colors.border, ui.colors.accent, t);

    dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), ImGui::ColorConvertFloat4ToU32(track_col), h * 0.5f);
    dl->AddRect(pos, ImVec2(pos.x + w, pos.y + h), ImGui::ColorConvertFloat4ToU32(border_col), h * 0.5f, 0, 1.0f);

    float r = (h - 6.0f) * 0.5f;
    float knob_x = pos.x + 3.0f + r + t * (w - 6.0f - 2.0f * r);
    float knob_y = pos.y + h * 0.5f;
    ImVec4 knob_col = *value ? ImVec4(1, 1, 1, 1) : lerp_col(ui.colors.text_sub, ImVec4(1, 1, 1, 1), t);
    dl->AddCircleFilled(ImVec2(knob_x, knob_y), r, ImGui::ColorConvertFloat4ToU32(knob_col));

    if (label && label[0]) {
        ImGui::SameLine(0, 10);
        ImGui::AlignTextToFramePadding();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, *value ? ui.colors.text_main : ui.colors.text_sub);
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();
        if (ImGui::IsItemClicked()) {
            *value = !(*value);
            clicked = true;
        }
    }
    return clicked;
}

bool begin_card(UiContext& ui, const char* id, float height) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ui.colors.bg_card);
    ImGui::PushStyleColor(ImGuiCol_Border, ui.colors.border);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 14));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);

    ImGuiChildFlags flags = ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding;
    if (height <= 0.0f) flags |= ImGuiChildFlags_AutoResizeY;

    return ImGui::BeginChild(id, ImVec2(-1, height), flags);
}

void end_card() {
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
    ImGui::Dummy(ImVec2(0, 6));
}

void section_title(UiContext& ui, const char* title, const char* hint) {
    ImGui::TextColored(ui.colors.text_main, "%s", title);
    if (hint && hint[0]) {
        ImGui::PushFont(ui.font_small);
        ImGui::PushStyleColor(ImGuiCol_Text, ui.colors.text_sub);
        ImGui::TextWrapped("%s", hint);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }
    ImGui::Dummy(ImVec2(0, 2));
}

void status_pill(const ThemeColors& c, const char* text, ImVec4 dot_color) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pad(10, 4);
    ImVec2 txt_sz = ImGui::CalcTextSize(text);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float w = pad.x * 2.0f + 12.0f + txt_sz.x;
    float h = pad.y * 2.0f + txt_sz.y;

    ImVec4 bg(dot_color.x, dot_color.y, dot_color.z, c.is_dark ? 0.16f : 0.12f);
    ImVec4 brd(dot_color.x, dot_color.y, dot_color.z, 0.45f);
    dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), ImGui::ColorConvertFloat4ToU32(bg), h * 0.5f);
    dl->AddRect(pos, ImVec2(pos.x + w, pos.y + h), ImGui::ColorConvertFloat4ToU32(brd), h * 0.5f);
    dl->AddCircleFilled(ImVec2(pos.x + pad.x + 4.0f, pos.y + h * 0.5f), 4.0f, ImGui::ColorConvertFloat4ToU32(dot_color));
    dl->AddText(ImVec2(pos.x + pad.x + 13.0f, pos.y + pad.y), ImGui::ColorConvertFloat4ToU32(c.text_main), text);
    ImGui::Dummy(ImVec2(w, h));
}

bool token_chip(UiContext& ui, const char* token, std::string& target_str, const char* tooltip) {
    ImGui::PushFont(ui.font_small);
    ImGui::PushStyleColor(ImGuiCol_Button, ui.colors.accent_soft);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ui.colors.accent.x, ui.colors.accent.y, ui.colors.accent.z, 0.30f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ui.colors.accent);
    ImGui::PushStyleColor(ImGuiCol_Text, ui.colors.accent);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(9, 4));
    std::string lbl = std::string("+") + token;
    bool clicked = ImGui::Button(lbl.c_str());
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
    ImGui::PopFont();

    if (tooltip && tooltip[0] && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip("%s", tooltip);
    }
    if (clicked) {
        if (!target_str.empty() && target_str.back() != ' ' && target_str.back() != ':') {
            target_str.push_back(' ');
        }
        target_str += token;
    }
    return clicked;
}

void copy_path_row(UiContext& ui, const char* id, const char* label, const std::string& path_str, const std::string& lang) {
    ImGui::PushID(id);
    ImGui::PushFont(ui.font_small);
    ImGui::TextColored(ui.colors.text_sub, "%s", label);
    ImGui::PopFont();

    const float btn_w = 106.0f;
    float avail = ImGui::GetContentRegionAvail().x;
    ImGui::SetNextItemWidth(std::max(120.0f, avail - btn_w - 8.0f));
    std::string ro = path_str;
    ImGui::InputText("##path", &ro, ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine(0, 8);

    double now = ImGui::GetTime();
    bool is_copied = (ui.copied_id == id && now < ui.copied_until);
    if (is_copied) {
        ImGui::PushStyleColor(ImGuiCol_Button, ui.colors.success);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ui.colors.success);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
        std::string clbl = std::string(ICON_FA_CHECK " ") + tr(lang, "copied");
        ImGui::Button(clbl.c_str(), ImVec2(btn_w, 0));
        ImGui::PopStyleColor(3);
        ui.animating = true;
    } else {
        std::string clbl = std::string(ICON_FA_COPY " ") + tr(lang, "copy");
        if (subtle_button(ui.colors, clbl.c_str(), ImVec2(btn_w, 0))) {
            ImGui::SetClipboardText(path_str.c_str());
            ui.copied_id = id;
            ui.copied_until = now + 1.8;
        }
    }
    ImGui::PopID();
}

void live_preview_box(UiContext& ui, const char* label, const std::string& text) {
    ImGui::PushFont(ui.font_small);
    ImGui::TextColored(ui.colors.text_sub, "%s", label);
    ImGui::PopFont();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ui.colors.bg_input);
    ImGui::PushStyleColor(ImGuiCol_Border, ui.colors.accent_soft);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    std::string cid = std::string("##prev_") + label;
    if (ImGui::BeginChild(cid.c_str(), ImVec2(-1, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding)) {
        if (text.empty()) {
            ImGui::TextColored(ui.colors.text_dim, "(empty)");
        } else {
            ImGui::TextWrapped("%s", text.c_str());
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

bool accent_button(const ThemeColors& c, const char* label, const ImVec2& size) {
    ImGui::PushStyleColor(ImGuiCol_Button, c.accent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, c.accent_hover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, c.accent_hover);
    ImGui::PushStyleColor(ImGuiCol_Border, c.accent);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    bool res = ImGui::Button(label, size);
    ImGui::PopStyleColor(5);
    return res;
}

bool subtle_button(const ThemeColors& c, const char* label, const ImVec2& size) {
    ImGui::PushStyleColor(ImGuiCol_Button, c.bg_input);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, c.bg_hover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, c.accent_soft);
    ImGui::PushStyleColor(ImGuiCol_Border, c.border);
    ImGui::PushStyleColor(ImGuiCol_Text, c.text_main);
    bool res = ImGui::Button(label, size);
    ImGui::PopStyleColor(5);
    return res;
}

bool danger_button(const ThemeColors& c, const char* label, const ImVec2& size) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(c.danger.x, c.danger.y, c.danger.z, 0.14f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(c.danger.x, c.danger.y, c.danger.z, 0.28f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, c.danger);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(c.danger.x, c.danger.y, c.danger.z, 0.45f));
    ImGui::PushStyleColor(ImGuiCol_Text, c.danger);
    bool res = ImGui::Button(label, size);
    ImGui::PopStyleColor(5);
    return res;
}

} // namespace osmv::gui
