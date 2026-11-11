#include "views.hpp"
#include "locale.hpp"
#include "common/formatter.hpp"
#include "common/os_utils.hpp"
#include "common/settings.hpp"
#include "IconsFontAwesome6.h"
#include "imgui_stdlib.h"

#include <algorithm>

namespace osmv::gui {

void render_view_now_playing(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    auto& ui = st.ui;
    const auto& c = ui.colors;

    page_header(ui, ICON_FA_MUSIC, tr(lang, "np_title"), tr(lang, "np_sub"));

    // ── Card 1: Live Player Preview & OBS Overlay Options ────────────────────
    if (begin_card(ui, "##np_main_card")) {
        if (toggle_switch(ui, "##np_en", tr(lang, "enable_module"), &st.settings.now_playing_enabled)) {
            st.mark_changed_immediate();
        }
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 4));

        // Album Art + Track Info
        const float art_sz = 96.0f;
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

        ImGui::SameLine(0, 16);
        ImGui::BeginGroup();
        {
            const std::string& status = st.snap.status;
            ImVec4 dot_col = c.text_dim;
            const char* st_lbl = tr(lang, "np_closed");
            if (status == "playing") { dot_col = c.success; st_lbl = tr(lang, "np_playing"); }
            else if (status == "paused") { dot_col = c.warning; st_lbl = tr(lang, "np_paused"); }
            else if (status == "stopped") { dot_col = c.danger; st_lbl = tr(lang, "np_stopped"); }
            status_pill(c, st_lbl, dot_col);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::PushFont(ui.font_title);
            const char* title_txt = st.snap.title.empty() ? tr(lang, "np_waiting") : st.snap.title.c_str();
            ImGui::TextWrapped("%s", title_txt);
            ImGui::PopFont();

            if (!st.snap.artist.empty() && st.snap.artist != "Unknown Artist") {
                ImGui::TextColored(c.text_sub, "%s", st.snap.artist.c_str());
            }
            if (!st.snap.album.empty()) {
                ImGui::PushFont(ui.font_small);
                ImGui::TextColored(c.text_dim, "%s: %s", tr(lang, "np_album"), st.snap.album.c_str());
                ImGui::PopFont();
            }
        }
        ImGui::EndGroup();

        ImGui::Dummy(ImVec2(0, 8));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 4));

        if (toggle_switch(ui, "##np_dyn", tr(lang, "np_dyn_color"), &st.settings.dynamic_color)) {
            st.mark_changed_immediate();
        }
        ImGui::SameLine(0, 24);
        if (toggle_switch(ui, "##np_viz", tr(lang, "np_viz"), &st.settings.audio_visualizer)) {
            st.mark_changed_immediate();
        }

        ImGui::Dummy(ImVec2(0, 6));
        copy_path_row(ui, "obs_html_path", tr(lang, "np_obs_html"),
                      os::path_to_utf8(cfg::obs_widget_html_path()), lang);
    }
    end_card();

    // ── Card 2: Snaz Text & JSON Exports ─────────────────────────────────────
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

} // namespace osmv::gui

