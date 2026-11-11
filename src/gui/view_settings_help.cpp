#include "views.hpp"
#include "locale.hpp"
#include "common/os_utils.hpp"
#include "common/settings.hpp"
#include "IconsFontAwesome6.h"
#include "imgui_stdlib.h"

#include <cstdio>

namespace osmv::gui {

void render_view_settings(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    auto& ui = st.ui;
    const auto& c = ui.colors;
    auto& g = st.settings.general;

    page_header(ui, ICON_FA_GEAR, tr(lang, "set_title"), tr(lang, "set_sub"));

    // ── Appearance & Language Card ───────────────────────────────────────────
    if (begin_card(ui, "##set_appearance")) {
        section_title(ui, tr(lang, "set_lang"));
        auto lang_btn = [&](const char* code, const char* label) {
            bool active = (g.language == code);
            if (active ? accent_button(c, label, ImVec2(120, 32)) : subtle_button(c, label, ImVec2(120, 32))) {
                g.language = code;
                st.mark_changed_immediate();
            }
        };
        lang_btn("fr", "Fran\xC3\xA7""ais");
        ImGui::SameLine();
        lang_btn("en", "English");

        ImGui::Dummy(ImVec2(0, 10));
        section_title(ui, tr(lang, "set_theme"));
        std::string dark_lbl = std::string(ICON_FA_MOON " ") + tr(lang, "set_dark");
        std::string light_lbl = std::string(ICON_FA_SUN " ") + tr(lang, "set_light");
        if (g.theme != "light" ? accent_button(c, dark_lbl.c_str(), ImVec2(120, 32)) : subtle_button(c, dark_lbl.c_str(), ImVec2(120, 32))) {
            g.theme = "dark";
            st.theme_changed = true;
            st.mark_changed_immediate();
        }
        ImGui::SameLine();
        if (g.theme == "light" ? accent_button(c, light_lbl.c_str(), ImVec2(120, 32)) : subtle_button(c, light_lbl.c_str(), ImVec2(120, 32))) {
            g.theme = "light";
            st.theme_changed = true;
            st.mark_changed_immediate();
        }

        ImGui::Dummy(ImVec2(0, 10));
        section_title(ui, tr(lang, "set_accent"));
        struct Swatch { const char* id; const char* name; ImVec4 col; };
        const Swatch swatches[] = {
            {"blurple", "Blurple", ImVec4(88 / 255.f, 101 / 255.f, 242 / 255.f, 1.f)},
            {"emerald", "Emerald", ImVec4(16 / 255.f, 185 / 255.f, 129 / 255.f, 1.f)},
            {"cyan",    "Cyan",    ImVec4(6 / 255.f, 182 / 255.f, 212 / 255.f, 1.f)},
            {"rose",    "Rose",    ImVec4(244 / 255.f, 63 / 255.f, 94 / 255.f, 1.f)},
            {"amber",   "Amber",   ImVec4(245 / 255.f, 158 / 255.f, 11 / 255.f, 1.f)}
        };
        for (int i = 0; i < 5; ++i) {
            if (i > 0) ImGui::SameLine(0, 10);
            ImGui::PushID(swatches[i].id);
            bool active = (g.accent == swatches[i].id);
            ImGui::PushStyleColor(ImGuiCol_Button, swatches[i].col);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, swatches[i].col);
            ImGui::PushStyleColor(ImGuiCol_Border, active ? c.text_main : swatches[i].col);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, active ? 2.5f : 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 16.0f);
            if (ImGui::Button(active ? ICON_FA_CHECK : "", ImVec2(32, 32))) {
                g.accent = swatches[i].id;
                st.theme_changed = true;
                st.mark_changed_immediate();
            }
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", swatches[i].name);
            ImGui::PopID();
        }
    }
    end_card();

    // ── Output Directory & Startup Card ──────────────────────────────────────
    if (begin_card(ui, "##set_paths")) {
        section_title(ui, tr(lang, "set_outdir"), tr(lang, "set_outdir_hint"));
        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(std::max(140.0f, avail - 135.0f));
        if (ImGui::InputTextWithHint("##out_dir", os::path_to_utf8(cfg::shared_dir() / "dynamic_text").c_str(), &g.output_dir)) {
            st.mark_changed_debounced();
        }
        ImGui::SameLine(0, 8);
        if (subtle_button(c, tr(lang, "set_reset_dir"), ImVec2(125, 0))) {
            g.output_dir.clear();
            st.mark_changed_immediate();
        }

        ImGui::Dummy(ImVec2(0, 8));
        if (toggle_switch(ui, "##set_opengui", tr(lang, "set_open_gui"), &g.open_gui_on_startup)) {
            st.mark_changed_immediate();
        }
        if (toggle_switch(ui, "##set_autostart", tr(lang, "set_autostart"), &g.autostart)) {
            os::set_autostart(g.autostart);
            st.mark_changed_immediate();
        }
    }
    end_card();
}

void render_view_help(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    auto& ui = st.ui;
    const auto& c = ui.colors;
    const auto dyn = cfg::dynamic_text_dir(st.settings);

    page_header(ui, ICON_FA_CIRCLE_QUESTION, tr(lang, "help_title"), tr(lang, "help_sub"));

    // ── About + Live RAM Card ────────────────────────────────────────────────
    if (begin_card(ui, "##help_about")) {
        if (st.logo_tex) {
            ImGui::Image(st.logo_tex, ImVec2(72, 72));
            ImGui::SameLine(0, 18);
        }
        ImGui::BeginGroup();
        {
            ImGui::PushFont(ui.font_title);
            ImGui::TextUnformatted("OBS Stream Music Viewer (OSMV)");
            ImGui::PopFont();
            ImGui::TextColored(c.accent, "Version 3.0.0 — C++20 Native");
            ImGui::Dummy(ImVec2(0, 2));
            ImGui::PushStyleColor(ImGuiCol_Text, c.text_sub);
            ImGui::TextWrapped("%s", tr(lang, "help_desc"));
            ImGui::PopStyleColor();
        }
        ImGui::EndGroup();

        ImGui::Dummy(ImVec2(0, 8));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 6));

        section_title(ui, tr(lang, "help_mem_title"));
        uint64_t gui_kb = os::get_process_memory_kb();
        char mem_buf[160];
        std::snprintf(mem_buf, sizeof(mem_buf), "%s: %.1f MB   |   %s: %.1f MB",
                      tr(lang, "daemon_ram"), st.snap.daemon_ram_kb / 1024.0,
                      tr(lang, "gui_ram"), gui_kb / 1024.0);
        status_pill(c, mem_buf, c.success);
    }
    end_card();

    // ── OBS Quick Setup Card ─────────────────────────────────────────────────
    if (begin_card(ui, "##help_obs")) {
        section_title(ui, tr(lang, "help_obs_title"));
        ImGui::PushStyleColor(ImGuiCol_Text, c.text_sub);
        ImGui::TextWrapped("%s", tr(lang, "help_obs_1"));
        ImGui::Dummy(ImVec2(0, 2));
        ImGui::TextWrapped("%s", tr(lang, "help_obs_2"));
        ImGui::Dummy(ImVec2(0, 2));
        ImGui::TextWrapped("%s", tr(lang, "help_obs_3"));
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 8));
        std::string open_lbl = std::string(ICON_FA_FOLDER_OPEN " ") + tr(lang, "open_folder");
        if (accent_button(c, open_lbl.c_str())) {
            os::open_folder(dyn);
        }
        ImGui::SameLine(0, 10);
        if (subtle_button(c, "shared/index.html")) {
            os::open_folder(cfg::shared_dir());
        }
    }
    end_card();
}

} // namespace osmv::gui

