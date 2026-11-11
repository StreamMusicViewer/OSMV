#include "views.hpp"
#include "locale.hpp"
#include "common/ipc.hpp"
#include "common/os_utils.hpp"
#include "common/settings.hpp"
#include "IconsFontAwesome6.h"
#include "imgui_stdlib.h"

#include <algorithm>
#include <sstream>

namespace osmv::gui {

namespace {
std::string join_lines(const std::vector<std::string>& lines) {
    std::string out;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) out.push_back('\n');
        out += lines[i];
    }
    return out;
}

std::vector<std::string> split_lines(const std::string& raw) {
    std::vector<std::string> out;
    std::istringstream iss(raw);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        out.push_back(std::move(line));
    }
    return out;
}
} // namespace

void render_view_text_files(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    auto& ui = st.ui;
    const auto& c = ui.colors;
    const auto dyn = cfg::dynamic_text_dir(st.settings);

    page_header(ui, ICON_FA_FILE_LINES, tr(lang, "tf_title"), tr(lang, "tf_sub"));

    // ── 1. Textline Changer ──────────────────────────────────────────────────
    if (begin_card(ui, "##tl_card")) {
        auto& tl = st.settings.textline;
        if (!st.textline_buf_init) {
            st.textline_buf = join_lines(tl.lines);
            st.textline_buf_init = true;
        }

        section_title(ui, tr(lang, "tl_card"));
        if (toggle_switch(ui, "##tl_en", tr(lang, "enable_module"), &tl.enabled)) {
            st.mark_changed_immediate();
        }
        ImGui::SameLine(0, 20);
        if (toggle_switch(ui, "##tl_rand", tr(lang, "tl_random"), &tl.random_order)) {
            st.mark_changed_immediate();
        }
        ImGui::SameLine(0, 20);
        if (toggle_switch(ui, "##tl_skip", tr(lang, "tl_skip_empty"), &tl.skip_empty)) {
            st.mark_changed_immediate();
        }

        ImGui::Dummy(ImVec2(0, 4));
        if (ImGui::BeginTable("##tl_opts", 3, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "tl_filename"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##tl_fn", &tl.filename)) st.mark_changed_debounced();

            ImGui::TableNextColumn();
            ImGui::PushFont(ui.font_small);
            ImGui::TextColored(c.text_sub, "%s", tr(lang, "interval_sec"));
            ImGui::PopFont();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputInt("##tl_iv", &tl.interval_sec, 1, 5)) {
                tl.interval_sec = std::clamp(tl.interval_sec, 1, 3600);
                st.mark_changed_immediate();
            }

            ImGui::TableNextColumn();
            ImGui::Dummy(ImVec2(0, 18));
            std::string nlbl = std::string(ICON_FA_FORWARD_STEP " ") + tr(lang, "tl_next");
            if (accent_button(c, nlbl.c_str(), ImVec2(-1, 0))) {
                std::string dummy;
                ipc::send_request(R"({"cmd":"textline_next"})", dummy, 300);
            }
            ImGui::EndTable();
        }

        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_sub, "%s", tr(lang, "tl_lines"));
        ImGui::PopFont();
        if (ImGui::InputTextMultiline("##tl_lines_edit", &st.textline_buf, ImVec2(-1, 100))) {
            tl.lines = split_lines(st.textline_buf);
            st.mark_changed_debounced();
        }

        live_preview_box(ui, tr(lang, "live_preview"), st.snap.textline_preview);
        std::string fn = tl.filename.empty() ? "TextlineChanger.txt" : tl.filename;
        copy_path_row(ui, "tl_txt", tr(lang, "file_path"), os::path_to_utf8(dyn / fn), lang);
    }
    end_card();

    // ── 2. Dynamic Files ─────────────────────────────────────────────────────
    if (begin_card(ui, "##df_card")) {
        auto& df = st.settings.dynamic_files;
        section_title(ui, tr(lang, "df_card"));
        if (toggle_switch(ui, "##df_en", tr(lang, "enable_module"), &df.enabled)) {
            st.mark_changed_immediate();
        }
        ImGui::SameLine(0, 24);
        std::string add_lbl = std::string(ICON_FA_PLUS " ") + tr(lang, "df_add");
        if (accent_button(c, add_lbl.c_str())) {
            DynamicFileRule r;
            r.filename = "Dynamic_" + std::to_string(df.rules.size() + 1) + ".txt";
            df.rules.push_back(std::move(r));
            st.mark_changed_immediate();
        }

        ImGui::Dummy(ImVec2(0, 6));
        int delete_idx = -1;
        for (size_t i = 0; i < df.rules.size(); ++i) {
            auto& r = df.rules[i];
            ImGui::PushID(static_cast<int>(i));

            ImGui::PushStyleColor(ImGuiCol_ChildBg, c.bg_input);
            ImGui::PushStyleColor(ImGuiCol_Border, c.border);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
            if (ImGui::BeginChild("##rule_box", ImVec2(-1, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding)) {
                if (toggle_switch(ui, "##r_en", tr(lang, "enabled"), &r.enabled)) {
                    st.mark_changed_immediate();
                }

                // Live status badge if available
                if (i < st.snap.dynamic_statuses.size()) {
                    const auto& rs = st.snap.dynamic_statuses[i];
                    ImGui::SameLine(0, 16);
                    status_pill(c, rs.status.c_str(), rs.ok ? c.success : c.danger);
                }

                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 95);
                std::string del_lbl = std::string(ICON_FA_TRASH " ") + tr(lang, "df_delete");
                if (danger_button(c, del_lbl.c_str(), ImVec2(95, 0))) {
                    delete_idx = static_cast<int>(i);
                }

                ImGui::Dummy(ImVec2(0, 4));
                if (ImGui::BeginTable("##r_fields", 3, ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn("c1", ImGuiTableColumnFlags_WidthStretch, 1.2f);
                    ImGui::TableSetupColumn("c2", ImGuiTableColumnFlags_WidthStretch, 0.8f);
                    ImGui::TableSetupColumn("c3", ImGuiTableColumnFlags_WidthStretch, 0.7f);

                    ImGui::TableNextColumn();
                    ImGui::PushFont(ui.font_small);
                    ImGui::TextColored(c.text_sub, "%s", tr(lang, "tl_filename"));
                    ImGui::PopFont();
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::InputText("##r_fn", &r.filename)) st.mark_changed_debounced();

                    ImGui::TableNextColumn();
                    ImGui::PushFont(ui.font_small);
                    ImGui::TextColored(c.text_sub, "Type");
                    ImGui::PopFont();
                    const char* types[2] = {tr(lang, "df_type_text"), tr(lang, "df_type_img")};
                    int t_idx = (r.type == "image") ? 1 : 0;
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::Combo("##r_type", &t_idx, types, 2)) {
                        r.type = (t_idx == 1) ? "image" : "text";
                        st.mark_changed_immediate();
                    }

                    ImGui::TableNextColumn();
                    ImGui::PushFont(ui.font_small);
                    ImGui::TextColored(c.text_sub, "%s", tr(lang, "interval_sec"));
                    ImGui::PopFont();
                    ImGui::SetNextItemWidth(-1);
                    if (ImGui::InputInt("##r_iv", &r.interval_sec, 5, 30)) {
                        r.interval_sec = std::clamp(r.interval_sec, 5, 86400);
                        st.mark_changed_immediate();
                    }
                    ImGui::EndTable();
                }

                ImGui::PushFont(ui.font_small);
                ImGui::TextColored(c.text_sub, "URL (http:// / https://)");
                ImGui::PopFont();
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("##r_url", &r.url)) st.mark_changed_debounced();

                ImGui::PushFont(ui.font_small);
                ImGui::TextColored(c.text_dim, "%s", tr(lang, "df_headers"));
                ImGui::PopFont();
                if (ImGui::InputTextMultiline("##r_hdrs", &r.headers, ImVec2(-1, 42))) {
                    st.mark_changed_debounced();
                }

                copy_path_row(ui, "rule_path", tr(lang, "file_path"),
                              os::path_to_utf8(dyn / r.filename), lang);
            }
            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);
            ImGui::Dummy(ImVec2(0, 4));
            ImGui::PopID();
        }
        if (delete_idx >= 0) {
            df.rules.erase(df.rules.begin() + delete_idx);
            st.mark_changed_immediate();
        }
    }
    end_card();
}

} // namespace osmv::gui

