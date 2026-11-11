#include "views.hpp"
#include "locale.hpp"
#include "common/formatter.hpp"
#include "common/ipc.hpp"
#include "common/os_utils.hpp"
#include "common/settings.hpp"
#include "IconsFontAwesome6.h"
#include "imgui_stdlib.h"

#include <algorithm>
#include <ctime>

namespace osmv::gui {

namespace {
void send_timer_cmd(const char* cmd_name, const char* action) {
    std::string req = std::string(R"({"cmd":")") + cmd_name + R"(","action":")" + action + R"("})";
    std::string resp;
    ipc::send_request(req, resp, 300);
}
} // namespace

void render_view_timers(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    auto& ui = st.ui;
    const auto& c = ui.colors;
    const auto dyn = cfg::dynamic_text_dir(st.settings);

    page_header(ui, ICON_FA_STOPWATCH, tr(lang, "tm_title"), tr(lang, "tm_sub"));

    // ── 1. Countdown (Target Date/Time) ──────────────────────────────────────
    if (begin_card(ui, "##cd_card")) {
        auto& cd = st.settings.countdown;
        section_title(ui, tr(lang, "cd_card"));
        if (toggle_switch(ui, "##cd_en", tr(lang, "enable_module"), &cd.enabled)) {
            st.mark_changed_immediate();
        }
        ImGui::SameLine(0, 24);
        if (toggle_switch(ui, "##cd_pad", tr(lang, "pad_zeros"), &cd.pad_zeros)) {
            st.mark_changed_immediate();
        }

        ImGui::Dummy(ImVec2(0, 4));
        if (ImGui::BeginTable("##cd_dt_tbl", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "cd_date"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(76);
            if (ImGui::InputInt("##cd_y", &cd.target_year, 0, 0)) { cd.target_year = std::clamp(cd.target_year, 2020, 2100); st.mark_changed_debounced(); }
            ImGui::SameLine(0, 4);
            ImGui::SetNextItemWidth(52);
            if (ImGui::InputInt("##cd_m", &cd.target_month, 0, 0)) { cd.target_month = std::clamp(cd.target_month, 1, 12); st.mark_changed_debounced(); }
            ImGui::SameLine(0, 4);
            ImGui::SetNextItemWidth(52);
            if (ImGui::InputInt("##cd_d", &cd.target_day, 0, 0)) { cd.target_day = std::clamp(cd.target_day, 1, 31); st.mark_changed_debounced(); }

            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "cd_time"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(52);
            if (ImGui::InputInt("##cd_hh", &cd.target_hour, 0, 0)) { cd.target_hour = std::clamp(cd.target_hour, 0, 23); st.mark_changed_debounced(); }
            ImGui::SameLine(0, 4);
            ImGui::SetNextItemWidth(52);
            if (ImGui::InputInt("##cd_mm", &cd.target_minute, 0, 0)) { cd.target_minute = std::clamp(cd.target_minute, 0, 59); st.mark_changed_debounced(); }
            ImGui::SameLine(0, 4);
            ImGui::SetNextItemWidth(52);
            if (ImGui::InputInt("##cd_ss", &cd.target_second, 0, 0)) { cd.target_second = std::clamp(cd.target_second, 0, 59); st.mark_changed_debounced(); }
            ImGui::EndTable();
        }

        auto set_from_offset = [&](int add_sec) {
            std::time_t t = std::time(nullptr) + add_sec;
            std::tm tm_v{};
#ifdef _WIN32
            localtime_s(&tm_v, &t);
#else
            localtime_r(&t, &tm_v);
#endif
            cd.target_year = tm_v.tm_year + 1900;
            cd.target_month = tm_v.tm_mon + 1;
            cd.target_day = tm_v.tm_mday;
            cd.target_hour = tm_v.tm_hour;
            cd.target_minute = tm_v.tm_min;
            cd.target_second = tm_v.tm_sec;
            st.mark_changed_immediate();
        };

        if (subtle_button(c, tr(lang, "cd_plus15"))) set_from_offset(15 * 60);
        ImGui::SameLine();
        if (subtle_button(c, tr(lang, "cd_plus1h"))) set_from_offset(3600);
        ImGui::SameLine();
        if (subtle_button(c, tr(lang, "cd_tonight"))) {
            std::tm tm_v = fmt::local_tm_now();
            cd.target_year = tm_v.tm_year + 1900;
            cd.target_month = tm_v.tm_mon + 1;
            cd.target_day = tm_v.tm_mday;
            cd.target_hour = 20;
            cd.target_minute = 0;
            cd.target_second = 0;
            st.mark_changed_immediate();
        }

        ImGui::Dummy(ImVec2(0, 4));
        if (ImGui::BeginTable("##cd_fmt_tbl", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "format"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##cd_fmt", &cd.format)) st.mark_changed_debounced();

            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "done_text"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##cd_done", &cd.done_text)) st.mark_changed_debounced();
            ImGui::EndTable();
        }

        if (token_chip(ui, "$d", cd.format, "Days")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$h", cd.format, "Hours")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$m", cd.format, "Minutes")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$s", cd.format, "Seconds")) st.mark_changed_immediate();

        auto cd_res = fmt::format_countdown_string(
            cd.target_year, cd.target_month, cd.target_day,
            cd.target_hour, cd.target_minute, cd.target_second,
            cd.format, cd.done_text, cd.pad_zeros);
        live_preview_box(ui, tr(lang, "live_preview"), cd_res.output);
        copy_path_row(ui, "countdown_txt", tr(lang, "file_path"), os::path_to_utf8(dyn / "Countdown.txt"), lang);
    }
    end_card();

    // ── 2. Chrono Down ───────────────────────────────────────────────────────
    if (begin_card(ui, "##cdown_card")) {
        auto& cdown = st.settings.chrono_down;
        section_title(ui, tr(lang, "cdown_card"));
        if (toggle_switch(ui, "##cdown_en", tr(lang, "enable_module"), &cdown.enabled)) {
            st.mark_changed_immediate();
        }
        ImGui::SameLine(0, 24);
        if (toggle_switch(ui, "##cdown_pad", tr(lang, "pad_zeros"), &cdown.pad_zeros)) {
            st.mark_changed_immediate();
        }
        ImGui::SameLine(0, 24);
        status_pill(c, st.snap.chrono_down_running ? tr(lang, "running") : tr(lang, "paused_lbl"),
                    st.snap.chrono_down_running ? c.success : c.text_dim);

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_sub, "%s", tr(lang, "duration_hms"));
        ImGui::PopFont();
        ImGui::SetNextItemWidth(60);
        if (ImGui::InputInt("##cdown_h", &cdown.initial_hours, 0, 0)) { cdown.initial_hours = std::clamp(cdown.initial_hours, 0, 999); st.mark_changed_immediate(); }
        ImGui::SameLine(0, 4);
        ImGui::SetNextItemWidth(60);
        if (ImGui::InputInt("##cdown_m", &cdown.initial_minutes, 0, 0)) { cdown.initial_minutes = std::clamp(cdown.initial_minutes, 0, 59); st.mark_changed_immediate(); }
        ImGui::SameLine(0, 4);
        ImGui::SetNextItemWidth(60);
        if (ImGui::InputInt("##cdown_s", &cdown.initial_seconds, 0, 0)) { cdown.initial_seconds = std::clamp(cdown.initial_seconds, 0, 59); st.mark_changed_immediate(); }

        ImGui::SameLine(0, 16);
        if (st.snap.chrono_down_running) {
            std::string plbl = std::string(ICON_FA_PAUSE " ") + tr(lang, "pause");
            if (subtle_button(c, plbl.c_str())) send_timer_cmd("chrono_down", "pause");
        } else {
            std::string slbl = std::string(ICON_FA_PLAY " ") + tr(lang, "start");
            if (accent_button(c, slbl.c_str())) send_timer_cmd("chrono_down", "start");
        }
        ImGui::SameLine();
        std::string rlbl = std::string(ICON_FA_ROTATE_LEFT " ") + tr(lang, "reset");
        if (subtle_button(c, rlbl.c_str())) send_timer_cmd("chrono_down", "reset");

        if (ImGui::BeginTable("##cdown_fmt_tbl", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "format"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##cdown_fmt", &cdown.format)) st.mark_changed_debounced();
            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "done_text"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##cdown_done", &cdown.done_text)) st.mark_changed_debounced();
            ImGui::EndTable();
        }

        if (token_chip(ui, "$h", cdown.format, "Hours")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$m", cdown.format, "Minutes")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$s", cdown.format, "Seconds")) st.mark_changed_immediate();

        std::string cdown_prev = !st.snap.chrono_down_preview.empty()
                                     ? st.snap.chrono_down_preview
                                     : fmt::format_chrono_down(
                                           (static_cast<int64_t>(cdown.initial_hours) * 3600LL +
                                            static_cast<int64_t>(cdown.initial_minutes) * 60LL +
                                            static_cast<int64_t>(cdown.initial_seconds)) * 1000LL,
                                           cdown.format, cdown.done_text, cdown.pad_zeros);
        live_preview_box(ui, tr(lang, "live_preview"), cdown_prev);
        copy_path_row(ui, "chrono_down_txt", tr(lang, "file_path"), os::path_to_utf8(dyn / "ChronoDown.txt"), lang);
    }
    end_card();

    // ── 3. Chrono Up ─────────────────────────────────────────────────────────
    if (begin_card(ui, "##cup_card")) {
        auto& cup = st.settings.chrono_up;
        section_title(ui, tr(lang, "cup_card"));
        if (toggle_switch(ui, "##cup_en", tr(lang, "enable_module"), &cup.enabled)) {
            st.mark_changed_immediate();
        }
        ImGui::SameLine(0, 24);
        if (toggle_switch(ui, "##cup_pad", tr(lang, "pad_zeros"), &cup.pad_zeros)) {
            st.mark_changed_immediate();
        }
        ImGui::SameLine(0, 24);
        status_pill(c, st.snap.chrono_up_running ? tr(lang, "running") : tr(lang, "paused_lbl"),
                    st.snap.chrono_up_running ? c.success : c.text_dim);

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_sub, "%s", tr(lang, "start_hms"));
        ImGui::PopFont();
        ImGui::SetNextItemWidth(60);
        if (ImGui::InputInt("##cup_h", &cup.start_hours, 0, 0)) { cup.start_hours = std::clamp(cup.start_hours, 0, 999); st.mark_changed_immediate(); }
        ImGui::SameLine(0, 4);
        ImGui::SetNextItemWidth(60);
        if (ImGui::InputInt("##cup_m", &cup.start_minutes, 0, 0)) { cup.start_minutes = std::clamp(cup.start_minutes, 0, 59); st.mark_changed_immediate(); }
        ImGui::SameLine(0, 4);
        ImGui::SetNextItemWidth(60);
        if (ImGui::InputInt("##cup_s", &cup.start_seconds, 0, 0)) { cup.start_seconds = std::clamp(cup.start_seconds, 0, 59); st.mark_changed_immediate(); }

        ImGui::SameLine(0, 16);
        if (st.snap.chrono_up_running) {
            std::string plbl = std::string(ICON_FA_PAUSE " ") + tr(lang, "pause");
            if (subtle_button(c, plbl.c_str())) send_timer_cmd("chrono_up", "pause");
        } else {
            std::string slbl = std::string(ICON_FA_PLAY " ") + tr(lang, "start");
            if (accent_button(c, slbl.c_str())) send_timer_cmd("chrono_up", "start");
        }
        ImGui::SameLine();
        std::string rlbl = std::string(ICON_FA_ROTATE_LEFT " ") + tr(lang, "reset");
        if (subtle_button(c, rlbl.c_str())) send_timer_cmd("chrono_up", "reset");

        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_sub, "%s", tr(lang, "format"));
        ImGui::PopFont();
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##cup_fmt", &cup.format)) st.mark_changed_debounced();

        if (token_chip(ui, "$d", cup.format, "Days")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$h", cup.format, "Hours")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$m", cup.format, "Minutes")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$s", cup.format, "Seconds")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$totalminutes", cup.format, "Total minutes")) st.mark_changed_immediate();

        std::string cup_prev = !st.snap.chrono_up_preview.empty()
                                   ? st.snap.chrono_up_preview
                                   : fmt::format_chrono_up(
                                         (static_cast<int64_t>(cup.start_hours) * 3600LL +
                                          static_cast<int64_t>(cup.start_minutes) * 60LL +
                                          static_cast<int64_t>(cup.start_seconds)) * 1000LL,
                                         cup.format, cup.pad_zeros);
        live_preview_box(ui, tr(lang, "live_preview"), cup_prev);
        copy_path_row(ui, "chrono_up_txt", tr(lang, "file_path"), os::path_to_utf8(dyn / "ChronoUp.txt"), lang);
    }
    end_card();
}

} // namespace osmv::gui

