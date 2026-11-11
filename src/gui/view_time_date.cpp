#include "views.hpp"
#include "locale.hpp"
#include "common/formatter.hpp"
#include "common/os_utils.hpp"
#include "common/settings.hpp"
#include "IconsFontAwesome6.h"
#include "imgui_stdlib.h"

namespace osmv::gui {

void render_view_time_date(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    auto& ui = st.ui;
    const auto& c = ui.colors;
    const auto dyn = cfg::dynamic_text_dir(st.settings);

    page_header(ui, ICON_FA_CLOCK, tr(lang, "td_title"), tr(lang, "td_sub"));

    // ── Time Module Card ─────────────────────────────────────────────────────
    if (begin_card(ui, "##time_card")) {
        section_title(ui, tr(lang, "time_card"));
        if (toggle_switch(ui, "##time_en", tr(lang, "enable_module"), &st.settings.time.enabled)) {
            st.mark_changed_immediate();
        }
        ImGui::SameLine(0, 24);
        if (toggle_switch(ui, "##time_ampm", tr(lang, "time_ampm"), &st.settings.time.use_ampm)) {
            st.mark_changed_immediate();
        }

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_sub, "%s", tr(lang, "format"));
        ImGui::PopFont();
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##time_fmt", &st.settings.time.format)) {
            st.mark_changed_debounced();
        }

        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_dim, "%s", tr(lang, "click_insert"));
        ImGui::PopFont();
        if (token_chip(ui, "$h", st.settings.time.format, "Hours")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$m", st.settings.time.format, "Minutes")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$s", st.settings.time.format, "Seconds")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$tt", st.settings.time.format, "AM / PM")) st.mark_changed_immediate();

        auto [pc_prev, live_prev] = fmt::format_time_string(st.settings.time.format, st.settings.time.use_ampm);
        ImGui::Dummy(ImVec2(0, 2));
        if (ImGui::BeginTable("##time_prev_tbl", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            live_preview_box(ui, tr(lang, "time_pc"), pc_prev);
            ImGui::TableNextColumn();
            live_preview_box(ui, tr(lang, "live_preview"), live_prev);
            ImGui::EndTable();
        }

        copy_path_row(ui, "time_txt", tr(lang, "file_path"),
                      os::path_to_utf8(dyn / "Time.txt"), lang);
    }
    end_card();

    // ── Date Module Card ─────────────────────────────────────────────────────
    if (begin_card(ui, "##date_card")) {
        section_title(ui, tr(lang, "date_card"));
        if (toggle_switch(ui, "##date_en", tr(lang, "enable_module"), &st.settings.date.enabled)) {
            st.mark_changed_immediate();
        }

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_sub, "%s", tr(lang, "date_preset"));
        ImGui::PopFont();

        static constexpr const char* kDatePresets[] = {
            "$dd-MM-yy",
            "$dd-MM-yyyy",
            "$dd MMMM yyyy",
            "$dddd dd MMMM yyyy"
        };
        int cur_preset = 3;
        for (int i = 0; i < 4; ++i) {
            if (st.settings.date.date_format == kDatePresets[i]) {
                cur_preset = i;
                break;
            }
        }
        ImGui::SetNextItemWidth(300);
        if (ImGui::Combo("##date_preset_combo", &cur_preset, kDatePresets, 4)) {
            st.settings.date.date_format = kDatePresets[cur_preset];
            st.mark_changed_immediate();
        }

        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_sub, "%s", tr(lang, "date_tpl"));
        ImGui::PopFont();
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##date_out_fmt", &st.settings.date.output_format)) {
            st.mark_changed_debounced();
        }
        if (token_chip(ui, "$date", st.settings.date.output_format, "Formatted date")) {
            st.mark_changed_immediate();
        }

        std::string date_prev = fmt::format_date_string(
            st.settings.date.date_format, st.settings.date.output_format, lang);
        live_preview_box(ui, tr(lang, "live_preview"), date_prev);

        copy_path_row(ui, "date_txt", tr(lang, "file_path"),
                      os::path_to_utf8(dyn / "Date.txt"), lang);
    }
    end_card();
}

} // namespace osmv::gui

