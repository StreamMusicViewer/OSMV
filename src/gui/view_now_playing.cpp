#include "views.hpp"
#include "locale.hpp"
#include "common/formatter.hpp"
#include "common/obs_widget.hpp"
#include "common/os_utils.hpp"
#include "common/settings.hpp"
#include "IconsFontAwesome6.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace osmv::gui {

namespace {

std::string to_file_url(const std::filesystem::path& p) {
    std::string u8 = os::path_to_utf8(p);
    std::replace(u8.begin(), u8.end(), '\\', '/');
    std::string out = (u8.size() >= 2 && u8[1] == ':') ? "file:///" : "file://";
    for (unsigned char ch : u8) {
        if (ch == ' ') out += "%20";
        else if (ch == '#') out += "%23";
        else if (ch == '%') out += "%25";
        else out.push_back(static_cast<char>(ch));
    }
    return out;
}

std::string url_encode_param(const std::string& raw) {
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string out;
    for (unsigned char ch : raw) {
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
            ch == '-' || ch == '_' || ch == '.') {
            out.push_back(static_cast<char>(ch));
        } else {
            out.push_back('%');
            out.push_back(kHex[(ch >> 4) & 0x0F]);
            out.push_back(kHex[ch & 0x0F]);
        }
    }
    return out;
}

void set_studio_feedback(GuiAppState& st, const char* msg) {
    st.obs_studio_feedback = msg;
    st.obs_studio_feedback_until = ImGui::GetTime() + 2.4;
}

void mark_widget_changed(GuiAppState& st, bool immediate) {
    st.settings.obs_widget.preset = "custom";
    if (immediate) st.mark_changed_immediate();
    else st.mark_changed_debounced();
}

bool color_hex_picker(const char* label, std::string& hex_str) {
    uint8_t r = 255, g = 255, b = 255, a = 255;
    obs::parse_hex_rgba(hex_str, r, g, b, a);
    float col[4] = {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
    if (ImGui::ColorEdit4(label, col, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs)) {
        auto to_u8 = [](float v) -> uint8_t {
            return static_cast<uint8_t>(std::clamp(std::lround(v * 255.0f), 0L, 255L));
        };
        hex_str = obs::format_hex_rgba(to_u8(col[0]), to_u8(col[1]), to_u8(col[2]), to_u8(col[3]));
        return true;
    }
    return false;
}

template <size_t N>
bool string_combo(const char* id, const char* label, std::string& value, const char* const (&keys)[N], const char* const (&labels)[N]) {
    int cur = 0;
    for (size_t i = 0; i < N; ++i) {
        if (value == keys[i]) {
            cur = static_cast<int>(i);
            break;
        }
    }
    bool changed = false;
    if (ImGui::BeginCombo(id, labels[cur])) {
        for (size_t i = 0; i < N; ++i) {
            bool sel = (cur == static_cast<int>(i));
            if (ImGui::Selectable(labels[i], sel)) {
                value = keys[i];
                changed = true;
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if (label && label[0]) {
        ImGui::SameLine(0, 8);
        ImGui::TextUnformatted(label);
    }
    return changed;
}

bool snippet_chip(UiContext& ui, const char* label, std::string& css, const char* snippet) {
    ImGui::PushFont(ui.font_small);
    ImGui::PushStyleColor(ImGuiCol_Button, ui.colors.accent_soft);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ui.colors.accent.x, ui.colors.accent.y, ui.colors.accent.z, 0.30f));
    ImGui::PushStyleColor(ImGuiCol_Text, ui.colors.accent);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(9, 4));
    bool clicked = ImGui::Button(label);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
    ImGui::PopFont();
    if (clicked) {
        ImGui::ClearActiveID();
        if (!css.empty() && css.back() != '\n') css += "\n";
        if (!css.empty()) css += "\n";
        css += snippet;
    }
    return clicked;
}

void render_playback_header_card(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    auto& ui = st.ui;
    const auto& c = ui.colors;
    const bool is_fr = (lang == "fr");

    if (!begin_card(ui, "##np_main_card")) {
        end_card();
        return;
    }

    if (toggle_switch(ui, "##np_en", tr(lang, "enable_module"), &st.settings.now_playing_enabled)) {
        st.mark_changed_immediate();
    }
    ImGui::SameLine();
    {
        float rem = ImGui::GetContentRegionAvail().x;
        if (rem > 175.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + rem - 168.0f);
    }
    std::string bprev_lbl = std::string(ICON_FA_ARROW_UP_RIGHT_FROM_SQUARE " ") + tr(lang, "obs_open_browser");
    if (accent_button(c, bprev_lbl.c_str(), ImVec2(168, 0))) {
        st.mark_changed_immediate();
        os::open_url_in_browser(to_file_url(cfg::obs_widget_preview_html_path()));
    }

    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 4));

    const float art_sz = 72.0f;
    ImVec2 art_pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(art_pos, ImVec2(art_pos.x + art_sz, art_pos.y + art_sz),
                      ImGui::ColorConvertFloat4ToU32(c.bg_input), 10.0f);
    dl->AddRect(art_pos, ImVec2(art_pos.x + art_sz, art_pos.y + art_sz),
                ImGui::ColorConvertFloat4ToU32(c.border), 10.0f);

    if (st.album_tex) {
        ImGui::Image(st.album_tex, ImVec2(art_sz, art_sz));
    } else {
        ImGui::Dummy(ImVec2(art_sz, art_sz));
        ImGui::PushFont(ui.font_title);
        ImVec2 icon_sz = ImGui::CalcTextSize(ICON_FA_COMPACT_DISC);
        dl->AddText(ui.font_title, ui.font_title->FontSize,
                    ImVec2(art_pos.x + (art_sz - icon_sz.x) * 0.5f, art_pos.y + (art_sz - icon_sz.y) * 0.5f),
                    ImGui::ColorConvertFloat4ToU32(c.text_dim), ICON_FA_COMPACT_DISC);
        ImGui::PopFont();
    }

    ImGui::SameLine(0, 14);
    ImGui::BeginGroup();
    {
        const std::string& status = st.snap.status;
        ImVec4 dot_col = c.text_dim;
        const char* st_lbl = tr(lang, "np_closed");
        if (status == "playing") { dot_col = c.success; st_lbl = tr(lang, "np_playing"); }
        else if (status == "paused") { dot_col = c.warning; st_lbl = tr(lang, "np_paused"); }
        else if (status == "stopped") { dot_col = c.danger; st_lbl = tr(lang, "np_stopped"); }
        status_pill(c, st_lbl, dot_col);
        if (!st.snap.player_name.empty()) {
            ImGui::SameLine(0, 8);
            ImGui::TextColored(c.text_dim, "(%s)", st.snap.player_name.c_str());
        }

        ImGui::Dummy(ImVec2(0, 2));
        ImGui::PushFont(ui.font_title);
        const char* title_txt = st.snap.title.empty() ? tr(lang, "np_waiting") : st.snap.title.c_str();
        ImGui::TextWrapped("%s", title_txt);
        ImGui::PopFont();

        if (!st.snap.artist.empty() && st.snap.artist != "Unknown Artist") {
            ImGui::PushStyleColor(ImGuiCol_Text, c.text_sub);
            ImGui::TextWrapped("%s%s%s", st.snap.artist.c_str(),
                               st.snap.album.empty() ? "" : "  -  ",
                               st.snap.album.c_str());
            ImGui::PopStyleColor();
        }
    }
    ImGui::EndGroup();

    ImGui::Dummy(ImVec2(0, 6));
    copy_path_row(ui, "obs_html_path", tr(lang, "np_obs_html"),
                  os::path_to_utf8(cfg::obs_widget_html_path()), lang);
    (void)is_fr;
    end_card();
}

void render_tab_style(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    const bool is_fr = (lang == "fr");
    auto& ui = st.ui;
    const auto& c = ui.colors;
    auto& w = st.settings.obs_widget;

    if (begin_card(ui, "##obs_presets_card")) {
        section_title(ui, tr(lang, "obs_presets"), tr(lang, "obs_presets_hint"));
        const auto& presets = obs::preset_catalog();
        float x_avail = ImGui::GetContentRegionAvail().x;
        int cols = std::max(2, std::min(4, static_cast<int>(x_avail / 150.0f)));
        if (ImGui::BeginTable("##preset_grid", cols, ImGuiTableFlags_SizingStretchSame)) {
            for (const auto& p : presets) {
                ImGui::TableNextColumn();
                bool active = (w.preset == p.id);
                const char* pname = is_fr ? p.name_fr : p.name_en;
                const char* pdesc = is_fr ? p.desc_fr : p.desc_en;
                if (active ? accent_button(c, pname, ImVec2(-1, 32)) : subtle_button(c, pname, ImVec2(-1, 32))) {
                    ImGui::ClearActiveID();
                    obs::apply_preset(w, p.id);
                    st.mark_changed_immediate();
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                    ImGui::SetTooltip("%s", pdesc);
                }
            }
            ImGui::EndTable();
        }

        ImGui::Dummy(ImVec2(0, 6));
        std::string reset_lbl = std::string(ICON_FA_ROTATE_LEFT "  ") + tr(lang, "obs_reset_defaults");
        if (danger_button(c, reset_lbl.c_str())) {
            ImGui::ClearActiveID();
            obs::reset_widget_defaults(st.settings);
            st.mark_changed_immediate();
            set_studio_feedback(st, tr(lang, "obs_reset_done"));
        }
        if (!st.obs_studio_feedback.empty() && ImGui::GetTime() < st.obs_studio_feedback_until) {
            ImGui::SameLine(0, 12);
            status_pill(c, st.obs_studio_feedback.c_str(), c.success);
            ui.animating = true;
        }
    }
    end_card();

    if (begin_card(ui, "##obs_saved_presets_card")) {
        section_title(ui, tr(lang, "obs_saved_presets"), tr(lang, "obs_saved_presets_hint"));
        const float btn_w = 170.0f;
        const float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(std::max(140.0f, avail - btn_w - 8.0f));
        ImGui::InputTextWithHint("##new_preset_name", tr(lang, "obs_preset_name"), &st.obs_new_preset_name);
        ImGui::SameLine(0, 8);

        std::string save_lbl = std::string(ICON_FA_FLOPPY_DISK "  ") + tr(lang, "obs_save_preset");
        if (accent_button(c, save_lbl.c_str(), ImVec2(btn_w, 0))) {
            std::string trimmed = st.obs_new_preset_name;
            while (!trimmed.empty() && trimmed.front() == ' ') trimmed.erase(trimmed.begin());
            while (!trimmed.empty() && trimmed.back() == ' ') trimmed.pop_back();
            if (!trimmed.empty() && obs::upsert_custom_preset(st.settings, trimmed)) {
                st.obs_new_preset_name.clear();
                st.mark_changed_immediate();
                set_studio_feedback(st, tr(lang, "obs_preset_saved_msg"));
            }
        }

        ImGui::Dummy(ImVec2(0, 6));
        if (st.settings.obs_custom_presets.empty()) {
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_dim, "%s", tr(lang, "obs_no_saved_presets"));
            ImGui::PopFont();
        } else {
            std::string to_delete;
            std::string to_apply;
            std::string to_update;
            const std::string base_url = to_file_url(cfg::obs_widget_html_path());

            for (size_t i = 0; i < st.settings.obs_custom_presets.size(); ++i) {
                const auto& cp = st.settings.obs_custom_presets[i];
                ImGui::PushID(static_cast<int>(i));
                const bool is_cur = (w.preset == ("user:" + cp.name));

                const float row_w = ImGui::GetContentRegionAvail().x;
                const float action_w = 274.0f;
                const float load_w = std::max(120.0f, row_w - action_w - 18.0f);
                if (is_cur ? accent_button(c, cp.name.c_str(), ImVec2(load_w, 28))
                           : subtle_button(c, cp.name.c_str(), ImVec2(load_w, 28))) {
                    to_apply = cp.name;
                }
                ImGui::SameLine(0, 6);
                std::string upd_lbl = std::string(ICON_FA_FLOPPY_DISK " ") + tr(lang, "obs_update_preset");
                if (subtle_button(c, upd_lbl.c_str(), ImVec2(110.0f, 28))) {
                    to_update = cp.name;
                }
                ImGui::SameLine(0, 6);
                std::string url_lbl = std::string(ICON_FA_COPY " ") + tr(lang, "obs_copy_scene_url");
                if (subtle_button(c, url_lbl.c_str(), ImVec2(114.0f, 28))) {
                    std::string scene_url = base_url + "?preset=" + url_encode_param(cp.name);
                    ImGui::SetClipboardText(scene_url.c_str());
                    set_studio_feedback(st, tr(lang, "obs_preset_url_copied"));
                }
                ImGui::SameLine(0, 6);
                if (danger_button(c, ICON_FA_TRASH, ImVec2(36.0f, 28))) {
                    to_delete = cp.name;
                }
                ImGui::PopID();
            }
            if (!to_apply.empty()) {
                ImGui::ClearActiveID();
                obs::apply_custom_preset(st.settings, to_apply);
                st.mark_changed_immediate();
            } else if (!to_update.empty()) {
                obs::upsert_custom_preset(st.settings, to_update);
                st.mark_changed_immediate();
                set_studio_feedback(st, tr(lang, "obs_preset_saved_msg"));
            } else if (!to_delete.empty()) {
                obs::delete_custom_preset(st.settings, to_delete);
                st.mark_changed_immediate();
            }
        }
    }
    end_card();

    if (begin_card(ui, "##obs_colors_card")) {
        section_title(ui, tr(lang, "obs_colors_card"));
        if (toggle_switch(ui, "##np_dyn", tr(lang, "np_dyn_color"), &st.settings.dynamic_color)) {
            st.mark_changed_immediate();
        }
        if (st.settings.dynamic_color) {
            ImGui::SameLine(0, 18);
            ImGui::SetNextItemWidth(160);
            static constexpr const char* kDynKeys[3] = {"card", "accent", "subtle"};
            const char* dyn_lbls_fr[3] = {"Carte complete", "Bordure & Accent", "Voile subtil"};
            const char* dyn_lbls_en[3] = {"Full card tint", "Border & Accent", "Subtle glaze"};
            if (string_combo("##dyn_mode", tr(lang, "obs_dyn_mode"), w.dynamic_color_mode,
                             kDynKeys, is_fr ? dyn_lbls_fr : dyn_lbls_en)) {
                mark_widget_changed(st, true);
            }
        }

        ImGui::Dummy(ImVec2(0, 6));
        if (ImGui::BeginTable("##col_grid", 3, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            if (color_hex_picker(tr(lang, "obs_bg_col"), w.bg_color)) mark_widget_changed(st, false);
            ImGui::TableNextColumn();
            if (color_hex_picker(tr(lang, "obs_border_col"), w.border_color)) mark_widget_changed(st, false);
            ImGui::TableNextColumn();
            if (color_hex_picker(tr(lang, "obs_accent_col"), w.accent_color)) mark_widget_changed(st, false);

            ImGui::TableNextColumn();
            if (color_hex_picker(tr(lang, "obs_title_col"), w.title_color)) mark_widget_changed(st, false);
            ImGui::TableNextColumn();
            if (color_hex_picker(tr(lang, "obs_artist_col"), w.artist_color)) mark_widget_changed(st, false);
            ImGui::TableNextColumn();
            if (color_hex_picker(tr(lang, "obs_album_col"), w.album_color)) mark_widget_changed(st, false);
            ImGui::EndTable();
        }

        ImGui::Dummy(ImVec2(0, 4));
        if (toggle_switch(ui, "##obs_grad", tr(lang, "obs_bg_grad"), &w.use_bg_gradient)) {
            mark_widget_changed(st, true);
        }
        if (w.use_bg_gradient) {
            ImGui::SameLine(0, 18);
            if (color_hex_picker(tr(lang, "obs_bg_col2"), w.bg_color_2)) mark_widget_changed(st, false);
        }

        ImGui::SameLine(0, 24);
        if (toggle_switch(ui, "##obs_glow", tr(lang, "obs_glow"), &w.glow_enabled)) {
            mark_widget_changed(st, true);
        }

        ImGui::Dummy(ImVec2(0, 4));
        if (ImGui::BeginTable("##fx_sliders", 3, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##blur_px", &w.backdrop_blur_px, 0, 32)) mark_widget_changed(st, false);
            ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_blur"), w.backdrop_blur_px);

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##brd_w", &w.border_width_px, 0, 6)) mark_widget_changed(st, false);
            ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_border_w"), w.border_width_px);

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat("##sh_int", &w.shadow_intensity, 0.0f, 1.0f, "%.2f")) mark_widget_changed(st, false);
            ImGui::TextColored(c.text_dim, "%s: %.2f", tr(lang, "obs_shadow"), w.shadow_intensity);
            ImGui::EndTable();
        }
    }
    end_card();
}

void render_tab_layout_content(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    const bool is_fr = (lang == "fr");
    auto& ui = st.ui;
    const auto& c = ui.colors;
    auto& w = st.settings.obs_widget;

    if (begin_card(ui, "##obs_geom_card")) {
        section_title(ui, tr(lang, "obs_layout_sec"));
        if (ImGui::BeginTable("##geom_combos", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            static constexpr const char* kPosKeys[5] = {"bottom-right", "bottom-left", "top-right", "top-left", "center"};
            const char* pos_fr[5] = {"Bas-Droite", "Bas-Gauche", "Haut-Droite", "Haut-Gauche", "Centre"};
            const char* pos_en[5] = {"Bottom-Right", "Bottom-Left", "Top-Right", "Top-Left", "Center"};
            ImGui::SetNextItemWidth(-1);
            if (string_combo("##pos_cmb", nullptr, w.position, kPosKeys, is_fr ? pos_fr : pos_en)) {
                st.mark_changed_immediate();
            }
            ImGui::TextColored(c.text_dim, "%s", tr(lang, "obs_position"));

            ImGui::TableNextColumn();
            static constexpr const char* kLayKeys[4] = {"horizontal", "horizontal-right", "vertical", "compact"};
            const char* lay_fr[4] = {"Horizontal (Pochette a gauche)", "Horizontal (Pochette a droite)", "Vertical (Centre)", "Compact"};
            const char* lay_en[4] = {"Horizontal (Art left)", "Horizontal (Art right)", "Vertical (Centered)", "Compact"};
            ImGui::SetNextItemWidth(-1);
            if (string_combo("##lay_cmb", nullptr, w.layout, kLayKeys, is_fr ? lay_fr : lay_en)) {
                mark_widget_changed(st, true);
            }
            ImGui::TextColored(c.text_dim, "%s", tr(lang, "obs_layout_dir"));
            ImGui::EndTable();
        }

        ImGui::Dummy(ImVec2(0, 6));
        if (ImGui::BeginTable("##geom_sliders", 3, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##w_px", &w.width_px, 260, 760)) mark_widget_changed(st, false);
            ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_width"), w.width_px);

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##pad_px", &w.padding_px, 4, 40)) mark_widget_changed(st, false);
            ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_padding"), w.padding_px);

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##gap_px", &w.gap_px, 4, 36)) mark_widget_changed(st, false);
            ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_gap"), w.gap_px);

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##rad_px", &w.border_radius_px, 0, 48)) mark_widget_changed(st, false);
            ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_radius"), w.border_radius_px);

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##mrg_px", &w.margin_px, 0, 80)) st.mark_changed_debounced();
            ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_margin"), w.margin_px);
            ImGui::TableNextColumn();
            ImGui::EndTable();
        }

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 4));
        section_title(ui, tr(lang, "obs_art_sec"));
        if (toggle_switch(ui, "##show_art", tr(lang, "obs_show_art"), &w.show_album_art)) mark_widget_changed(st, true);
        if (w.show_album_art) {
            ImGui::SameLine(0, 18);
            if (toggle_switch(ui, "##spin_art", tr(lang, "obs_spin_art"), &w.spin_artwork)) mark_widget_changed(st, true);
            ImGui::Dummy(ImVec2(0, 4));
            if (ImGui::BeginTable("##art_sliders", 2, ImGuiTableFlags_SizingStretchSame)) {
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-1);
                if (ImGui::SliderInt("##art_sz", &w.art_size_px, 32, 150)) mark_widget_changed(st, false);
                ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_art_size"), w.art_size_px);

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-1);
                if (ImGui::SliderInt("##art_rad", &w.art_radius_px, 0, 75)) mark_widget_changed(st, false);
                ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_art_radius"), w.art_radius_px);
                ImGui::EndTable();
            }
        }
    }
    end_card();

    if (begin_card(ui, "##obs_content_card")) {
        section_title(ui, tr(lang, "obs_content_sec"));
        if (toggle_switch(ui, "##sh_t", tr(lang, "obs_show_title"), &w.show_title)) mark_widget_changed(st, true);
        ImGui::SameLine(0, 14);
        if (toggle_switch(ui, "##sh_a", tr(lang, "obs_show_artist"), &w.show_artist)) mark_widget_changed(st, true);
        ImGui::SameLine(0, 14);
        if (toggle_switch(ui, "##sh_al", tr(lang, "obs_show_album"), &w.show_album)) mark_widget_changed(st, true);
        ImGui::SameLine(0, 14);
        if (toggle_switch(ui, "##sh_bg", tr(lang, "obs_show_badge"), &w.show_status_badge)) mark_widget_changed(st, true);

        ImGui::Dummy(ImVec2(0, 6));
        auto fmt_field = [&](const char* fid, const char* lbl, bool active, std::string& dst) {
            if (!active) return;
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", lbl);
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText(fid, &dst)) mark_widget_changed(st, false);
        };
        fmt_field("##fmt_t", tr(lang, "obs_title_fmt"), w.show_title, w.title_format);
        fmt_field("##fmt_a", tr(lang, "obs_artist_fmt"), w.show_artist, w.artist_format);
        fmt_field("##fmt_al", tr(lang, "obs_album_fmt"), w.show_album, w.album_format);
        fmt_field("##fmt_b", tr(lang, "obs_badge_fmt"), w.show_status_badge, w.badge_format);

        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_dim, "%s", tr(lang, "click_insert"));
        ImGui::PopFont();
        std::string& chip_target = w.show_title ? w.title_format : w.artist_format;
        if (token_chip(ui, "$song", chip_target)) mark_widget_changed(st, true);
        ImGui::SameLine();
        if (token_chip(ui, "$artist", chip_target)) mark_widget_changed(st, true);
        ImGui::SameLine();
        if (token_chip(ui, "$album", chip_target)) mark_widget_changed(st, true);
        ImGui::SameLine();
        if (token_chip(ui, "$player", chip_target)) mark_widget_changed(st, true);
        ImGui::SameLine();
        if (token_chip(ui, "$status_upper", chip_target)) mark_widget_changed(st, true);

        ImGui::Dummy(ImVec2(0, 6));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 4));
        section_title(ui, tr(lang, "obs_typo_sec"));

        static constexpr const char* kFonts[8] = {
            "Segoe UI", "Inter", "Poppins", "Montserrat", "Roboto", "Outfit", "Space Grotesk", "JetBrains Mono"
        };
        ImGui::SetNextItemWidth(220);
        if (string_combo("##font_cmb", tr(lang, "obs_font"), w.font_family, kFonts, kFonts)) {
            mark_widget_changed(st, true);
        }
        ImGui::SameLine(0, 18);
        if (toggle_switch(ui, "##gfont", tr(lang, "obs_gfont"), &w.load_google_font)) mark_widget_changed(st, true);

        if (ImGui::BeginTable("##typo_sliders", 3, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##t_sz", &w.title_size_px, 12, 34)) mark_widget_changed(st, false);
            ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_title_sz"), w.title_size_px);

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##a_sz", &w.artist_size_px, 10, 26)) mark_widget_changed(st, false);
            ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_artist_sz"), w.artist_size_px);

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##al_sz", &w.album_size_px, 9, 22)) mark_widget_changed(st, false);
            ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_album_sz"), w.album_size_px);

            ImGui::TableNextColumn();
            static constexpr const char* kAlignKeys[3] = {"left", "center", "right"};
            const char* aln_fr[3] = {"Gauche", "Centre", "Droite"};
            const char* aln_en[3] = {"Left", "Center", "Right"};
            ImGui::SetNextItemWidth(-1);
            if (string_combo("##aln_cmb", nullptr, w.text_align, kAlignKeys, is_fr ? aln_fr : aln_en)) {
                mark_widget_changed(st, true);
            }
            ImGui::TextColored(c.text_dim, "%s", tr(lang, "obs_align"));

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##twt", &w.title_weight, 400, 800)) mark_widget_changed(st, false);
            ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_title_wt"), w.title_weight);

            ImGui::TableNextColumn();
            static constexpr const char* kTrKeys[5] = {"fade", "slide-up", "slide-left", "zoom", "none"};
            const char* tr_lbl[5] = {"Fade", "Slide Up", "Slide Left", "Zoom", "None"};
            ImGui::SetNextItemWidth(-1);
            if (string_combo("##tr_cmb", nullptr, w.transition_style, kTrKeys, tr_lbl)) {
                mark_widget_changed(st, true);
            }
            ImGui::TextColored(c.text_dim, "%s", tr(lang, "obs_trans"));
            ImGui::EndTable();
        }

        ImGui::Dummy(ImVec2(0, 4));
        if (toggle_switch(ui, "##tshad", tr(lang, "obs_text_shadow"), &w.text_shadow)) mark_widget_changed(st, true);
        ImGui::SameLine(0, 18);
        if (toggle_switch(ui, "##marq", tr(lang, "obs_marquee"), &w.marquee_scroll)) mark_widget_changed(st, true);
        if (w.marquee_scroll) {
            ImGui::SameLine(0, 14);
            ImGui::SetNextItemWidth(130);
            if (ImGui::SliderFloat("##mspd", &w.scroll_speed, 0.5f, 2.0f, "%.1fx")) mark_widget_changed(st, false);
        }

        ImGui::Dummy(ImVec2(0, 4));
        if (toggle_switch(ui, "##h_idle", tr(lang, "obs_hide_idle"), &w.hide_when_idle)) st.mark_changed_immediate();
        if (w.hide_when_idle) {
            ImGui::SameLine(0, 14);
            ImGui::SetNextItemWidth(130);
            if (ImGui::SliderInt("##h_del", &w.hide_delay_sec, 0, 60)) st.mark_changed_debounced();
            ImGui::SameLine(0, 6);
            ImGui::TextColored(c.text_dim, "%s", tr(lang, "obs_hide_delay"));
        } else {
            ImGui::SetNextItemWidth(280);
            if (ImGui::InputText(tr(lang, "obs_idle_txt"), &w.idle_text)) st.mark_changed_debounced();
        }
    }
    end_card();
}

void render_tab_visualizer(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    const bool is_fr = (lang == "fr");
    auto& ui = st.ui;
    const auto& c = ui.colors;
    auto& w = st.settings.obs_widget;

    if (begin_card(ui, "##obs_viz_card")) {
        section_title(ui, tr(lang, "np_viz"));
        if (toggle_switch(ui, "##np_viz_tab", tr(lang, "np_viz"), &st.settings.audio_visualizer)) {
            st.mark_changed_immediate();
        }

        ImGui::Dummy(ImVec2(0, 8));
        if (ImGui::BeginTable("##viz_combos", 3, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            static constexpr const char* kVPosKeys[3] = {"top", "bottom", "background"};
            const char* vp_fr[3] = {"Au-dessus", "En-dessous", "Arriere-plan carte"};
            const char* vp_en[3] = {"Above widget", "Below widget", "Card background"};
            ImGui::SetNextItemWidth(-1);
            if (string_combo("##vpos", nullptr, w.viz_position, kVPosKeys, is_fr ? vp_fr : vp_en)) {
                mark_widget_changed(st, true);
            }
            ImGui::TextColored(c.text_dim, "%s", tr(lang, "obs_viz_pos"));

            ImGui::TableNextColumn();
            static constexpr const char* kVStKeys[5] = {"bars", "mirrored", "dots", "wave", "blocks"};
            const char* vs_fr[5] = {"Barres classiques", "Barres symetriques", "Points flottants", "Onde fluide", "Blocs LED"};
            const char* vs_en[5] = {"Classic bars", "Mirrored EQ", "Floating dots", "Smooth wave", "LED blocks"};
            ImGui::SetNextItemWidth(-1);
            if (string_combo("##vst", nullptr, w.viz_style, kVStKeys, is_fr ? vs_fr : vs_en)) {
                mark_widget_changed(st, true);
            }
            ImGui::TextColored(c.text_dim, "%s", tr(lang, "obs_viz_style"));

            ImGui::TableNextColumn();
            static constexpr const char* kVCmKeys[3] = {"album", "custom", "rainbow"};
            const char* vc_fr[3] = {"Couleur album / accent", "Degrade perso", "Arc-en-ciel"};
            const char* vc_en[3] = {"Album / Accent hue", "Custom gradient", "Rainbow"};
            ImGui::SetNextItemWidth(-1);
            if (string_combo("##vcm", nullptr, w.viz_color_mode, kVCmKeys, is_fr ? vc_fr : vc_en)) {
                mark_widget_changed(st, true);
            }
            ImGui::TextColored(c.text_dim, "%s", tr(lang, "obs_viz_cmode"));
            ImGui::EndTable();
        }

        if (w.viz_color_mode == "custom") {
            ImGui::Dummy(ImVec2(0, 6));
            if (color_hex_picker(tr(lang, "obs_viz_ctop"), w.viz_color_top)) mark_widget_changed(st, false);
            ImGui::SameLine(0, 18);
            if (color_hex_picker(tr(lang, "obs_viz_cbot"), w.viz_color_bottom)) mark_widget_changed(st, false);
        }

        ImGui::Dummy(ImVec2(0, 8));
        if (ImGui::BeginTable("##viz_sliders", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##v_bars", &w.viz_bar_count, 8, 48)) mark_widget_changed(st, false);
            ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_viz_bars"), w.viz_bar_count);

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##v_h", &w.viz_height_px, 20, 120)) mark_widget_changed(st, false);
            ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_viz_h"), w.viz_height_px);

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##v_gap", &w.viz_gap_px, 1, 10)) mark_widget_changed(st, false);
            ImGui::TextColored(c.text_dim, "%s: %d", tr(lang, "obs_viz_gap"), w.viz_gap_px);

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat("##v_spd", &w.viz_speed, 0.4f, 2.0f, "%.1fx")) mark_widget_changed(st, false);
            ImGui::TextColored(c.text_dim, "%s: %.1fx", tr(lang, "obs_viz_speed"), w.viz_speed);
            ImGui::EndTable();
        }
    }
    end_card();
}

void render_tab_custom_css(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    auto& ui = st.ui;
    const auto& c = ui.colors;
    auto& w = st.settings.obs_widget;

    if (begin_card(ui, "##obs_css_card")) {
        section_title(ui, tr(lang, "obs_tab_css"), tr(lang, "obs_css_hint"));
        if (toggle_switch(ui, "##css_en", tr(lang, "obs_css_enable"), &w.custom_css_enabled)) {
            st.mark_changed_immediate();
        }
        ImGui::SameLine(0, 18);
        if (toggle_switch(ui, "##css_raw", tr(lang, "obs_css_raw"), &w.custom_css_raw_override)) {
            st.mark_changed_immediate();
        }

        ImGui::Dummy(ImVec2(0, 6));
        if (accent_button(c, tr(lang, "obs_css_load_studio"))) {
            ImGui::ClearActiveID();
            w.custom_css = obs::generate_studio_css(st.settings);
            w.custom_css_enabled = true;
            st.mark_changed_immediate();
            set_studio_feedback(st, tr(lang, "obs_css_loaded_msg"));
        }
        ImGui::SameLine(0, 8);
        if (subtle_button(c, tr(lang, "obs_css_copy_studio"))) {
            std::string generated = obs::generate_studio_css(st.settings);
            ImGui::SetClipboardText(generated.c_str());
            set_studio_feedback(st, tr(lang, "obs_css_copied_msg"));
        }
        ImGui::SameLine(0, 8);
        if (danger_button(c, tr(lang, "obs_css_clear"))) {
            ImGui::ClearActiveID();
            w.custom_css.clear();
            st.mark_changed_immediate();
        }

        if (!st.obs_studio_feedback.empty() && ImGui::GetTime() < st.obs_studio_feedback_until) {
            ImGui::SameLine(0, 12);
            status_pill(c, st.obs_studio_feedback.c_str(), c.success);
            ui.animating = true;
        }

        ImGui::Dummy(ImVec2(0, 6));
        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_dim, "%s", tr(lang, "obs_css_chips"));
        ImGui::PopFont();
        if (snippet_chip(ui, "+.widget-container", w.custom_css, ".widget-container {\n    background: rgba(12, 14, 22, 0.90);\n    border-radius: 18px;\n}")) {
            w.custom_css_enabled = true;
            st.mark_changed_immediate();
        }
        ImGui::SameLine();
        if (snippet_chip(ui, "+.album-art", w.custom_css, ".album-art {\n    border-radius: 50%;\n    box-shadow: 0 0 20px rgba(124, 92, 255, 0.5);\n}")) {
            w.custom_css_enabled = true;
            st.mark_changed_immediate();
        }
        ImGui::SameLine();
        if (snippet_chip(ui, "+.track-title", w.custom_css, ".track-title {\n    font-size: 22px;\n    letter-spacing: 0.02em;\n    color: #ffffff;\n}")) {
            w.custom_css_enabled = true;
            st.mark_changed_immediate();
        }
        ImGui::SameLine();
        if (snippet_chip(ui, "+.track-badge", w.custom_css, ".track-badge {\n    background: #7c5cff;\n    border-radius: 6px;\n}")) {
            w.custom_css_enabled = true;
            st.mark_changed_immediate();
        }

        ImGui::Dummy(ImVec2(0, 6));
        if (render_css_code_editor(ui, "##css_editor", w.custom_css, lang.c_str())) {
            st.mark_changed_debounced();
        }
    }
    end_card();
}

void render_tab_snaz(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    auto& ui = st.ui;
    const auto& c = ui.colors;

    if (begin_card(ui, "##np_snaz_card")) {
        section_title(ui, tr(lang, "np_snaz_exports"));

        auto& np = st.settings.now_playing;
        if (toggle_switch(ui, "##np_txt_files", tr(lang, "np_export_txt"), &np.export_text_files)) {
            st.mark_changed_immediate();
        }
        if (toggle_switch(ui, "##np_fmt_file", tr(lang, "np_export_fmt"), &np.export_formatted_file)) {
            st.mark_changed_immediate();
        }
        if (toggle_switch(ui, "##np_snaz_json", tr(lang, "np_export_json"), &np.export_snaz_json)) {
            st.mark_changed_immediate();
        }
        if (toggle_switch(ui, "##np_clear_idle", tr(lang, "np_clear_idle"), &np.clear_when_inactive)) {
            st.mark_changed_immediate();
        }

        if (!np.clear_when_inactive) {
            ImGui::SetNextItemWidth(260);
            if (ImGui::InputText(tr(lang, "np_fallback"), &np.fallback_text)) {
                st.mark_changed_debounced();
            }
        }

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 4));

        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_sub, "%s", tr(lang, "format"));
        ImGui::PopFont();
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##np_custom_fmt", &np.custom_format)) {
            st.mark_changed_debounced();
        }

        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_dim, "%s", tr(lang, "click_insert"));
        ImGui::PopFont();
        if (token_chip(ui, "$artist", np.custom_format, "Artist")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$song", np.custom_format, "Track title")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$album", np.custom_format, "Album")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$artist_upper", np.custom_format, "ARTIST")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$song_upper", np.custom_format, "TITLE")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$album_upper", np.custom_format, "ALBUM")) st.mark_changed_immediate();

        std::string preview = !st.snap.playing_now_preview.empty()
                                  ? st.snap.playing_now_preview
                                  : fmt::format_now_playing(
                                        st.snap.title.empty() ? "Song Title" : st.snap.title,
                                        st.snap.artist.empty() ? "Artist Name" : st.snap.artist,
                                        st.snap.album.empty() ? "Album Name" : st.snap.album,
                                        np.custom_format);
        live_preview_box(ui, tr(lang, "live_preview"), preview);

        const auto dyn = cfg::dynamic_text_dir(st.settings);
        copy_path_row(ui, "playing_now_txt", tr(lang, "file_path"),
                      os::path_to_utf8(dyn / "PlayingNow.txt"), lang);
    }
    end_card();
}

} // namespace

void render_view_now_playing(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    auto& ui = st.ui;
    const auto& c = ui.colors;

    page_header(ui, ICON_FA_MUSIC, tr(lang, "np_title"), tr(lang, "np_sub"));
    render_playback_header_card(st);

    // Pinned Live OBS Preview Card
    if (begin_card(ui, "##obs_preview_card")) {
        render_obs_widget_preview(st);
    }
    end_card();

    // Studio Navigation Tabs
    const char* tab_keys[5] = {
        "obs_tab_style", "obs_tab_layout", "obs_tab_viz", "obs_tab_css", "obs_tab_snaz"
    };
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 7));
    for (int i = 0; i < 5; ++i) {
        if (i > 0) ImGui::SameLine(0, 6);
        bool active = (st.obs_studio_tab == i);
        if (active ? accent_button(c, tr(lang, tab_keys[i])) : subtle_button(c, tr(lang, tab_keys[i]))) {
            st.obs_studio_tab = i;
        }
    }
    ImGui::PopStyleVar(2);
    ImGui::Dummy(ImVec2(0, 8));

    switch (st.obs_studio_tab) {
        case 0: render_tab_style(st); break;
        case 1: render_tab_layout_content(st); break;
        case 2: render_tab_visualizer(st); break;
        case 3: render_tab_custom_css(st); break;
        case 4: render_tab_snaz(st); break;
        default: render_tab_style(st); break;
    }
}

} // namespace osmv::gui
