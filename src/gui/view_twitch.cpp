#include "views.hpp"
#include "locale.hpp"
#include "common/formatter.hpp"
#include "common/ipc.hpp"
#include "common/os_utils.hpp"
#include "common/settings.hpp"
#include "IconsFontAwesome6.h"
#include "imgui_stdlib.h"
#include <nlohmann/json.hpp>

#include <algorithm>

namespace osmv::gui {

void render_view_twitch(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    auto& ui = st.ui;
    const auto& c = ui.colors;
    auto& tw = st.settings.twitch;
    const auto dyn = cfg::dynamic_text_dir(st.settings);

    if (!st.twitch_title_synced && !st.snap.twitch.current_title.empty()) {
        st.twitch_title_input = st.snap.twitch.current_title;
        st.twitch_title_synced = true;
    }

    page_header(ui, ICON_FA_TV, tr(lang, "tw_title"), tr(lang, "tw_sub"));

    // ── 1. Credentials Card ──────────────────────────────────────────────────
    if (begin_card(ui, "##tw_creds_card")) {
        if (toggle_switch(ui, "##tw_en", tr(lang, "enable_module"), &tw.enabled)) {
            st.mark_changed_immediate();
        }
        ImGui::SameLine(0, 24);
        ImVec4 dot_col = c.text_dim;
        if (tw.enabled) {
            dot_col = st.snap.twitch.connected ? (st.snap.twitch.is_live ? c.success : c.accent) : c.warning;
        }
        status_pill(c, st.snap.twitch.status_text.c_str(), dot_col);

        ImGui::Dummy(ImVec2(0, 6));
        section_title(ui, tr(lang, "tw_creds"));

        if (ImGui::BeginTable("##tw_cred_tbl", 3, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("c1", ImGuiTableColumnFlags_WidthStretch, 1.2f);
            ImGui::TableSetupColumn("c2", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("c3", ImGuiTableColumnFlags_WidthStretch, 0.7f);

            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "tw_client_id"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##tw_cid", &tw.client_id)) st.mark_changed_debounced();

            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "tw_channel"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##tw_chan", &tw.channel)) st.mark_changed_debounced();

            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "interval_sec"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputInt("##tw_iv", &tw.interval_sec, 15, 60)) {
                tw.interval_sec = std::clamp(tw.interval_sec, 15, 3600);
                st.mark_changed_immediate();
            }
            ImGui::EndTable();
        }

        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_sub, "%s", tr(lang, "tw_token"));
        ImGui::PopFont();
        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(std::max(120.0f, avail - 215.0f));
        ImGuiInputTextFlags tok_flags = st.show_twitch_token ? 0 : ImGuiInputTextFlags_Password;
        if (ImGui::InputText("##tw_tok", &tw.oauth_token, tok_flags)) {
            if (tw.oauth_token.find("access_token=") != std::string::npos ||
                tw.oauth_token.rfind("oauth:", 0) == 0) {
                tw.oauth_token = fmt::extract_twitch_token(tw.oauth_token);
            }
            st.mark_changed_debounced();
        }
        ImGui::SameLine(0, 6);
        if (subtle_button(c, st.show_twitch_token ? ICON_FA_EYE_SLASH : ICON_FA_EYE, ImVec2(36, 0))) {
            st.show_twitch_token = !st.show_twitch_token;
        }
        ImGui::SameLine(0, 6);
        std::string gtok = std::string(ICON_FA_KEY " ") + tr(lang, "tw_get_token");
        if (subtle_button(c, gtok.c_str(), ImVec2(166, 0))) {
            if (!tw.client_id.empty()) {
                if (!st.twitch_oauth) {
                    st.twitch_oauth = std::make_unique<TwitchOAuthListener>();
                }
                if (st.twitch_oauth->start(lang)) {
                    st.twitch_oauth_ok = true;
                    st.twitch_oauth_msg = tr(lang, "tw_oauth_wait");
                } else {
                    st.twitch_oauth_ok = false;
                    st.twitch_oauth_msg = tr(lang, "tw_oauth_fallback");
                }
                std::string url = "https://id.twitch.tv/oauth2/authorize?response_type=token"
                                  "&client_id=" + fmt::url_encode(tw.client_id) +
                                  "&redirect_uri=http://localhost&scope=channel:manage:broadcast";
                os::open_url_in_browser(url);
            } else {
                os::open_url_in_browser("https://dev.twitch.tv/console/apps");
            }
        }
        if (!st.twitch_oauth_msg.empty()) {
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(st.twitch_oauth_ok ? c.success : c.warning, "%s", st.twitch_oauth_msg.c_str());
            ImGui::PopFont();
        }
    }
    end_card();

    // ── 2. Viewer Count Card ─────────────────────────────────────────────────
    if (begin_card(ui, "##tw_view_card")) {
        section_title(ui, tr(lang, "tw_viewers_card"));
        if (ImGui::BeginTable("##tw_vfmt_tbl", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "format"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##tw_vfmt", &tw.viewers_format)) st.mark_changed_debounced();

            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "tw_offline_txt"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##tw_off", &tw.offline_text)) st.mark_changed_debounced();
            ImGui::EndTable();
        }
        if (token_chip(ui, "$viewers", tw.viewers_format, "Live viewer count")) st.mark_changed_immediate();

        std::string prev = fmt::format_twitch_viewers(
            st.snap.twitch.viewers, st.snap.twitch.is_live, tw.viewers_format, tw.offline_text);
        live_preview_box(ui, tr(lang, "live_preview"), prev);
        copy_path_row(ui, "tw_viewers_txt", tr(lang, "file_path"), os::path_to_utf8(dyn / "TwitchViewers.txt"), lang);
    }
    end_card();

    // ── 3. Title & Category Updater ──────────────────────────────────────────
    if (begin_card(ui, "##tw_upd_card")) {
        section_title(ui, tr(lang, "tw_update_card"));
        if (!st.snap.twitch.current_game.empty()) {
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "Current: %s — %s",
                               st.snap.twitch.current_game.c_str(), st.snap.twitch.current_title.c_str());
            ImGui::PopFont();
        }

        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_sub, "%s", tr(lang, "tw_stream_title"));
        ImGui::PopFont();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##tw_stitle", &st.twitch_title_input);

        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_sub, "%s", tr(lang, "tw_search_game"));
        ImGui::PopFont();
        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(std::max(120.0f, avail - 130.0f));
        bool enter_search = ImGui::InputText("##tw_gsearch", &st.twitch_game_search,
                                             ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine(0, 8);
        std::string slbl = std::string(ICON_FA_MAGNIFYING_GLASS " ") + tr(lang, "tw_search_btn");
        if (subtle_button(c, slbl.c_str(), ImVec2(120, 0)) || enter_search) {
            nlohmann::json req = {{"cmd", "twitch_search_games"}, {"query", st.twitch_game_search}};
            std::string resp;
            if (ipc::send_request(req.dump(), resp, 5000)) {
                try {
                    auto j = nlohmann::json::parse(resp);
                    st.twitch_search_results.clear();
                    if (j.contains("games") && j["games"].is_array()) {
                        for (const auto& g : j["games"]) {
                            st.twitch_search_results.push_back({g.value("id", ""), g.value("name", "")});
                        }
                    }
                } catch (...) {
                }
            }
        }

        if (!st.twitch_search_results.empty()) {
            for (size_t i = 0; i < st.twitch_search_results.size(); ++i) {
                const auto& g = st.twitch_search_results[i];
                bool sel = (st.twitch_selected_game_id == g.id);
                if (ImGui::Selectable((g.name + "  [" + g.id + "]").c_str(), sel)) {
                    st.twitch_selected_game_id = g.id;
                    st.twitch_selected_game_name = g.name;
                }
            }
        }

        if (!st.twitch_selected_game_name.empty()) {
            ImGui::TextColored(c.accent, "%s [%s]",
                               st.twitch_selected_game_name.c_str(),
                               st.twitch_selected_game_id.c_str());
            ImGui::SameLine(0, 12);
            if (subtle_button(c, tr(lang, "tw_add_preset"))) {
                tw.custom_games.push_back({st.twitch_selected_game_id, st.twitch_selected_game_name});
                st.mark_changed_immediate();
            }
        }

        if (!tw.custom_games.empty()) {
            ImGui::Dummy(ImVec2(0, 4));
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_dim, "%s", tr(lang, "tw_presets"));
            ImGui::PopFont();
            int del_g = -1;
            for (size_t i = 0; i < tw.custom_games.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));
                if (i > 0 && (i % 3) != 0) ImGui::SameLine();
                if (subtle_button(c, tw.custom_games[i].name.c_str())) {
                    st.twitch_selected_game_id = tw.custom_games[i].id;
                    st.twitch_selected_game_name = tw.custom_games[i].name;
                }
                ImGui::SameLine(0, 2);
                if (danger_button(c, "x")) del_g = static_cast<int>(i);
                ImGui::PopID();
            }
            if (del_g >= 0) {
                tw.custom_games.erase(tw.custom_games.begin() + del_g);
                st.mark_changed_immediate();
            }
        }

        ImGui::Dummy(ImVec2(0, 6));
        if (accent_button(c, tr(lang, "tw_apply_btn"))) {
            nlohmann::json req = {
                {"cmd", "twitch_update_channel"},
                {"title", st.twitch_title_input},
                {"game_id", st.twitch_selected_game_id}
            };
            std::string resp;
            if (ipc::send_request(req.dump(), resp, 6000)) {
                try {
                    auto j = nlohmann::json::parse(resp);
                    st.twitch_feedback_ok = j.value("ok", false);
                    st.twitch_feedback = st.twitch_feedback_ok ? tr(lang, "saved_ok") : j.value("error", "Error");
                } catch (...) {
                    st.twitch_feedback_ok = false;
                    st.twitch_feedback = "Error";
                }
            }
        }
        if (!st.twitch_feedback.empty()) {
            ImGui::SameLine(0, 12);
            ImGui::TextColored(st.twitch_feedback_ok ? c.success : c.danger, "%s", st.twitch_feedback.c_str());
        }
    }
    end_card();
}

} // namespace osmv::gui

