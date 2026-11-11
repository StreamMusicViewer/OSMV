#include "views.hpp"
#include "locale.hpp"
#include "common/formatter.hpp"
#include "common/os_utils.hpp"
#include "common/settings.hpp"
#include "IconsFontAwesome6.h"
#include "imgui_stdlib.h"

#include <cstdio>

namespace osmv::gui {

void render_view_sysinfo(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    auto& ui = st.ui;
    const auto& c = ui.colors;
    const auto& m = st.snap.sys_metrics;
    const auto dyn = cfg::dynamic_text_dir(st.settings);

    page_header(ui, ICON_FA_MICROCHIP, tr(lang, "si_title"), tr(lang, "si_sub"));

    // ── Live Metrics Summary Grid ────────────────────────────────────────────
    if (ImGui::BeginTable("##si_metrics_grid", 4, ImGuiTableFlags_SizingStretchSame)) {
        auto metric_tile = [&](const char* id, const char* icon, const char* lbl, const char* val_str, ImVec4 tint) {
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, c.bg_card);
            ImGui::PushStyleColor(ImGuiCol_Border, c.border);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
            if (ImGui::BeginChild(id, ImVec2(-1, 74), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding)) {
                ImGui::PushFont(ui.font_small);
                ImGui::TextColored(tint, "%s  %s", icon, lbl);
                ImGui::PopFont();
                ImGui::Dummy(ImVec2(0, 4));
                ImGui::PushFont(ui.font_title);
                ImGui::TextUnformatted(val_str);
                ImGui::PopFont();
            }
            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);
        };

        char cpu_s[32], ram_s[32], proc_s[32], net_s[64];
        std::snprintf(cpu_s, sizeof(cpu_s), "%.0f %%", m.cpu_percent);
        std::snprintf(ram_s, sizeof(ram_s), "%.0f %%", m.ram_in_use_percent);
        std::snprintf(proc_s, sizeof(proc_s), "%u", m.process_count);
        std::string down = fmt::format_rate_kbps(m.download_bytes_per_sec);
        std::snprintf(net_s, sizeof(net_s), "%s", down.c_str());

        metric_tile("##m_cpu", ICON_FA_MICROCHIP, tr(lang, "si_cpu"), cpu_s, c.accent);
        metric_tile("##m_ram", ICON_FA_MEMORY, tr(lang, "si_ram"), ram_s, c.success);
        metric_tile("##m_proc", ICON_FA_DESKTOP, tr(lang, "si_procs"), proc_s, c.warning);
        metric_tile("##m_net", ICON_FA_NETWORK_WIRED, tr(lang, "si_net"), net_s, c.accent);
        ImGui::EndTable();
    }
    ImGui::Dummy(ImVec2(0, 6));

    // ── Config Card ──────────────────────────────────────────────────────────
    if (begin_card(ui, "##si_cfg_card")) {
        auto& si = st.settings.system_info;
        if (toggle_switch(ui, "##si_en", tr(lang, "enable_module"), &si.enabled)) {
            st.mark_changed_immediate();
        }
        ImGui::SameLine(0, 28);
        ImGui::SetNextItemWidth(160);
        if (ImGui::SliderInt(tr(lang, "interval_sec"), &si.interval_sec, 1, 30)) {
            st.mark_changed_debounced();
        }

        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_sub, "%s", tr(lang, "format"));
        ImGui::PopFont();
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputTextMultiline("##si_fmt", &si.format, ImVec2(-1, 64))) {
            st.mark_changed_debounced();
        }

        ImGui::PushFont(ui.font_small);
        ImGui::TextColored(c.text_dim, "%s", tr(lang, "click_insert"));
        ImGui::PopFont();
        if (token_chip(ui, "$CPU_USAGE", si.format, "CPU usage (%)")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$RAM_MB_AVAIL", si.format, "Available RAM (MB)")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$RAM_%_IN_USE", si.format, "RAM used (%)")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$RAM_%_AVAIL", si.format, "RAM free (%)")) st.mark_changed_immediate();

        if (token_chip(ui, "$AMOUNT_PROCESSES", si.format, "Running processes")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$UPLOAD", si.format, "Network upload")) st.mark_changed_immediate();
        ImGui::SameLine();
        if (token_chip(ui, "$DOWNLOAD", si.format, "Network download")) st.mark_changed_immediate();

        std::string preview = fmt::format_system_info(m, si.format);
        live_preview_box(ui, tr(lang, "live_preview"), preview);
        copy_path_row(ui, "sysinfo_txt", tr(lang, "file_path"), os::path_to_utf8(dyn / "SystemInfo.txt"), lang);
    }
    end_card();
}

} // namespace osmv::gui

