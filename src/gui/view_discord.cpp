#include "views.hpp"
#include "locale.hpp"
#include "common/os_utils.hpp"
#include "IconsFontAwesome6.h"
#include "imgui_stdlib.h"

namespace osmv::gui {

void render_view_discord(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    auto& ui = st.ui;
    const auto& c = ui.colors;
    auto& dc = st.settings.discord;

    page_header(ui, ICON_FA_GAMEPAD, tr(lang, "dc_title"), tr(lang, "dc_sub"));

    // ── Header Enable + Status Card ──────────────────────────────────────────
    if (begin_card(ui, "##dc_top_card")) {
        if (toggle_switch(ui, "##dc_en", tr(lang, "enable_module"), &dc.enabled)) {
            st.mark_changed_immediate();
        }
        ImGui::SameLine(0, 24);
        ImVec4 dot_col = c.text_dim;
        const char* st_txt = tr(lang, "disabled");
        if (dc.enabled) {
            if (st.snap.discord_status == "connected") { dot_col = c.success; st_txt = tr(lang, "dc_connected"); }
            else if (st.snap.discord_status == "connecting") { dot_col = c.warning; st_txt = tr(lang, "dc_connecting"); }
            else if (st.snap.discord_status == "invalid_id") { dot_col = c.danger; st_txt = tr(lang, "dc_invalid"); }
            else if (st.snap.discord_status == "error") { dot_col = c.danger; st_txt = tr(lang, "dc_error"); }
            else { dot_col = c.warning; st_txt = tr(lang, "dc_waiting"); }
        }
        status_pill(c, st_txt, dot_col);

        ImGui::Dummy(ImVec2(0, 6));
        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_sub, "%s", tr(lang, "dc_client_id"));
        ImGui::PopFont();

        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(std::max(140.0f, avail - 235.0f));
        if (ImGui::InputTextWithHint("##dc_cid", "1234567890123456789", &dc.client_id)) {
            st.mark_changed_debounced();
        }
        ImGui::SameLine(0, 8);
        std::string portal_lbl = std::string(ICON_FA_LINK " ") + tr(lang, "dc_dev_portal");
        if (subtle_button(c, portal_lbl.c_str(), ImVec2(225, 0))) {
            os::open_url_in_browser("https://discord.com/developers/applications");
        }
    }
    end_card();

    // ── Music & Fallback Activity ────────────────────────────────────────────
    if (begin_card(ui, "##dc_act_card")) {
        if (toggle_switch(ui, "##dc_music", tr(lang, "dc_use_music"), &dc.use_music_when_playing)) {
            st.mark_changed_immediate();
        }
        ImGui::Dummy(ImVec2(0, 6));
        section_title(ui, tr(lang, "dc_idle_sec"));

        if (ImGui::BeginTable("##dc_idle_tbl", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "dc_details"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputTextWithHint("##dc_det", "Streaming on OBS", &dc.custom_details)) {
                st.mark_changed_debounced();
            }

            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "dc_state"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputTextWithHint("##dc_st", "Chilling with viewers", &dc.custom_state)) {
                st.mark_changed_debounced();
            }
            ImGui::EndTable();
        }
    }
    end_card();

    // ── Art Assets ───────────────────────────────────────────────────────────
    if (begin_card(ui, "##dc_assets_card")) {
        section_title(ui, tr(lang, "dc_assets"));

        if (ImGui::BeginTable("##dc_lg_tbl", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "dc_large_key"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputTextWithHint("##dc_lk", "osmv_logo", &dc.large_image_key)) st.mark_changed_debounced();

            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "dc_large_txt"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputTextWithHint("##dc_lt", "OSMV", &dc.large_image_text)) st.mark_changed_debounced();
            ImGui::EndTable();
        }

        ImGui::Dummy(ImVec2(0, 4));
        if (ImGui::BeginTable("##dc_status_keys_tbl", 3, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.success, "%s", tr(lang, "dc_key_play"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputTextWithHint("##dc_kp", "playing", &dc.status_key_playing)) st.mark_changed_debounced();

            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.warning, "%s", tr(lang, "dc_key_pause"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputTextWithHint("##dc_kpa", "paused", &dc.status_key_paused)) st.mark_changed_debounced();

            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.danger, "%s", tr(lang, "dc_key_stop"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputTextWithHint("##dc_ks", "stopped", &dc.status_key_stopped)) st.mark_changed_debounced();
            ImGui::EndTable();
        }

        ImGui::Dummy(ImVec2(0, 4));
        if (ImGui::BeginTable("##dc_sm_tbl", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "dc_small_key"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputTextWithHint("##dc_sk", "idle_icon", &dc.small_image_key)) st.mark_changed_debounced();

            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "dc_small_txt"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputTextWithHint("##dc_stxt", "Idle", &dc.small_image_text)) st.mark_changed_debounced();
            ImGui::EndTable();
        }
    }
    end_card();
}

} // namespace osmv::gui

