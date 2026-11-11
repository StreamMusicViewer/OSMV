#include "views.hpp"
#include "locale.hpp"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace osmv::gui {

namespace {

// VS Code Dark+ inspired CSS palette
constexpr ImU32 kColDefault   = IM_COL32(216, 222, 233, 255);
constexpr ImU32 kColComment   = IM_COL32(106, 153, 85, 255);
constexpr ImU32 kColAtRule    = IM_COL32(197, 134, 192, 255);
constexpr ImU32 kColSelector  = IM_COL32(215, 186, 125, 255);
constexpr ImU32 kColClassId   = IM_COL32(78, 201, 176, 255);
constexpr ImU32 kColProperty  = IM_COL32(156, 220, 254, 255);
constexpr ImU32 kColString    = IM_COL32(206, 145, 120, 255);
constexpr ImU32 kColNumber    = IM_COL32(181, 206, 168, 255);
constexpr ImU32 kColHexFunc   = IM_COL32(220, 220, 170, 255);
constexpr ImU32 kColColorHex  = IM_COL32(210, 168, 255, 255);
constexpr ImU32 kColPunct     = IM_COL32(139, 148, 158, 255);

bool is_ident_char(unsigned char ch) {
    return std::isalnum(ch) != 0 || ch == '-' || ch == '_' || ch >= 0x80;
}

bool is_hex_digit(unsigned char ch) {
    return std::isxdigit(ch) != 0;
}

struct CssScanState {
    bool in_comment = false;
    int brace_depth = 0;
    bool after_colon = false;
};

void draw_highlighted_line(ImDrawList* dl,
                           ImFont* font,
                           float font_size,
                           ImVec2 line_origin,
                           const char* line_begin,
                           const char* line_end,
                           CssScanState& st) {
    const char* p = line_begin;
    auto emit = [&](const char* tok_begin, const char* tok_end, ImU32 col) {
        if (tok_end <= tok_begin) return;
        float dx = (tok_begin > line_begin)
                       ? font->CalcTextSizeA(font_size, 100000.0f, -1.0f, line_begin, tok_begin).x
                       : 0.0f;
        dl->AddText(font, font_size, ImVec2(line_origin.x + dx, line_origin.y), col, tok_begin, tok_end);
    };

    while (p < line_end) {
        if (st.in_comment) {
            const char* close = nullptr;
            for (const char* q = p; q + 1 < line_end; ++q) {
                if (q[0] == '*' && q[1] == '/') {
                    close = q + 2;
                    break;
                }
            }
            if (close) {
                emit(p, close, kColComment);
                p = close;
                st.in_comment = false;
            } else {
                emit(p, line_end, kColComment);
                p = line_end;
            }
            continue;
        }

        unsigned char ch = static_cast<unsigned char>(*p);
        if (ch == ' ' || ch == '\t' || ch == '\r') {
            ++p;
            continue;
        }

        // Start of comment
        if (ch == '/' && (p + 1 < line_end) && p[1] == '*') {
            st.in_comment = true;
            continue;
        }

        // Quoted strings
        if (ch == '"' || ch == '\'') {
            char quote = *p;
            const char* q = p + 1;
            while (q < line_end) {
                if (*q == '\\' && q + 1 < line_end) {
                    q += 2;
                    continue;
                }
                if (*q == quote) {
                    ++q;
                    break;
                }
                ++q;
            }
            emit(p, q, kColString);
            p = q;
            continue;
        }

        // Braces / punctuation
        if (ch == '{') {
            emit(p, p + 1, kColPunct);
            ++st.brace_depth;
            st.after_colon = false;
            ++p;
            continue;
        }
        if (ch == '}') {
            emit(p, p + 1, kColPunct);
            st.brace_depth = std::max(0, st.brace_depth - 1);
            st.after_colon = false;
            ++p;
            continue;
        }
        if (ch == ';') {
            emit(p, p + 1, kColPunct);
            st.after_colon = false;
            ++p;
            continue;
        }
        if (ch == ':') {
            if (st.brace_depth > 0 && !st.after_colon) {
                emit(p, p + 1, kColPunct);
                st.after_colon = true;
                ++p;
                continue;
            }
            // Selector pseudo-class / pseudo-element (:hover, ::after)
            const char* q = p;
            while (q < line_end && (*q == ':' || is_ident_char(static_cast<unsigned char>(*q)))) ++q;
            emit(p, q, kColAtRule);
            p = q;
            continue;
        }
        if (ch == '(' || ch == ')' || ch == ',' || ch == '>' || ch == '+' || ch == '~' || ch == '*') {
            emit(p, p + 1, kColPunct);
            ++p;
            continue;
        }

        // !important
        if (ch == '!') {
            const char* q = p + 1;
            while (q < line_end && is_ident_char(static_cast<unsigned char>(*q))) ++q;
            emit(p, q, kColAtRule);
            p = q;
            continue;
        }

        // @at-rule
        if (ch == '@') {
            const char* q = p + 1;
            while (q < line_end && is_ident_char(static_cast<unsigned char>(*q))) ++q;
            emit(p, q, kColAtRule);
            p = q;
            continue;
        }

        // Selector .class or #id or hex color inside property values
        if (ch == '.' && st.brace_depth == 0) {
            const char* q = p + 1;
            while (q < line_end && is_ident_char(static_cast<unsigned char>(*q))) ++q;
            emit(p, q, kColClassId);
            p = q;
            continue;
        }
        if (ch == '#') {
            const char* q = p + 1;
            if (st.brace_depth > 0) {
                while (q < line_end && is_hex_digit(static_cast<unsigned char>(*q))) ++q;
                emit(p, q, kColColorHex);
            } else {
                while (q < line_end && is_ident_char(static_cast<unsigned char>(*q))) ++q;
                emit(p, q, kColClassId);
            }
            p = q;
            continue;
        }

        // Numbers and CSS units inside values (or keyframe percentages outside)
        bool looks_num = (std::isdigit(ch) != 0) ||
                         ((ch == '-' || ch == '.') && (p + 1 < line_end) &&
                          (std::isdigit(static_cast<unsigned char>(p[1])) != 0));
        if (looks_num && (st.brace_depth == 0 || st.after_colon)) {
            const char* q = p + 1;
            while (q < line_end && (std::isdigit(static_cast<unsigned char>(*q)) != 0 || *q == '.')) ++q;
            while (q < line_end && (std::isalpha(static_cast<unsigned char>(*q)) != 0 || *q == '%')) ++q;
            emit(p, q, kColNumber);
            p = q;
            continue;
        }

        // Identifiers (properties, functions, tag selectors, keywords)
        if (is_ident_char(ch)) {
            const char* q = p + 1;
            while (q < line_end && is_ident_char(static_cast<unsigned char>(*q))) ++q;
            ImU32 col = kColDefault;
            if (st.brace_depth == 0) {
                col = kColSelector;
            } else if (!st.after_colon) {
                col = kColProperty;
            } else if (q < line_end && *q == '(') {
                col = kColHexFunc;
            }
            emit(p, q, col);
            p = q;
            continue;
        }

        emit(p, p + 1, kColPunct);
        ++p;
    }
}

void compute_cursor_line_col(const std::string& text, int cursor_byte, int& out_line, int& out_col, const char*& out_line_start) {
    int clamped = std::clamp(cursor_byte, 0, static_cast<int>(text.size()));
    out_line = 1;
    out_col = 1;
    out_line_start = text.data();
    for (int i = 0; i < clamped; ++i) {
        if (text[static_cast<size_t>(i)] == '\n') {
            ++out_line;
            out_col = 1;
            out_line_start = text.data() + i + 1;
        } else if ((static_cast<unsigned char>(text[static_cast<size_t>(i)]) & 0xC0) != 0x80) {
            ++out_col;
        }
    }
}

} // namespace

bool render_css_code_editor(UiContext& ui, const char* id, std::string& css, const char* lang) {
    const auto& c = ui.colors;
    ImFont* code_font = ui.font_code ? ui.font_code : ui.font_regular;
    ImGui::PushFont(code_font);

    const float font_size = ImGui::GetFontSize();
    const float line_h = font_size;
    int line_count = 1;
    for (char ch : css) {
        if (ch == '\n') ++line_count;
    }

    const float pad_x = 12.0f;
    const float pad_y = 10.0f;
    const float avail_w = std::max(300.0f, ImGui::GetContentRegionAvail().x);
    const float gutter_w = (line_count >= 1000) ? 54.0f : 44.0f;
    const float code_w = std::max(200.0f, avail_w - gutter_w);
    const float editor_h = std::max(360.0f, static_cast<float>(line_count + 2) * line_h + pad_y * 2.0f + 6.0f);
    const float status_h = 26.0f;

    ImGuiWindow* parent_win = ImGui::GetCurrentWindow();
    ImDrawList* parent_dl = ImGui::GetWindowDrawList();
    const ImVec2 panel_p0 = ImGui::GetCursorScreenPos();
    const ImVec2 panel_p1(panel_p0.x + avail_w, panel_p0.y + editor_h + status_h);
    const ImVec2 code_p0(panel_p0.x + gutter_w, panel_p0.y);
    const ImVec2 code_p1(panel_p0.x + avail_w, panel_p0.y + editor_h);

    // Outer dark IDE panel + gutter
    parent_dl->AddRectFilled(panel_p0, panel_p1, IM_COL32(11, 14, 20, 255), 8.0f);
    parent_dl->AddRectFilled(panel_p0, ImVec2(code_p0.x, panel_p0.y + editor_h), IM_COL32(16, 21, 33, 255), 8.0f,
                             ImDrawFlags_RoundCornersTopLeft);
    parent_dl->AddLine(ImVec2(code_p0.x, panel_p0.y), ImVec2(code_p0.x, code_p1.y), IM_COL32(34, 42, 60, 255), 1.0f);

    // Inspect cursor before InputTextMultiline so active-line highlight sits behind selection
    const ImGuiID ed_id = ImGui::GetID(id);
    ImGuiInputTextState* pre_state = ImGui::GetInputTextState(ed_id);
    int cur_line = -1;
    int cur_col = 1;
    if (pre_state && ImGui::GetActiveID() == ed_id) {
        const char* dummy_start = nullptr;
        compute_cursor_line_col(css, pre_state->GetCursorPos(), cur_line, cur_col, dummy_start);
        float ly = code_p0.y + pad_y + static_cast<float>(cur_line - 1) * line_h - pre_state->Scroll.y;
        parent_dl->PushClipRect(code_p0, code_p1, true);
        parent_dl->AddRectFilled(ImVec2(code_p0.x + 1.0f, ly), ImVec2(code_p1.x - 1.0f, ly + line_h),
                                 IM_COL32(25, 33, 50, 190));
        parent_dl->PopClipRect();
    }

    // Reserve gutter space beside InputTextMultiline
    ImGui::Dummy(ImVec2(gutter_w, editor_h));
    ImGui::SameLine(0, 0);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_x, pad_y));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, IM_COL32(72, 122, 255, 88));

    bool changed = ImGui::InputTextMultiline(id, &css, ImVec2(code_w, editor_h), ImGuiInputTextFlags_AllowTabInput);

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);

    // Draw line numbers in gutter
    const int child_count = parent_win->DC.ChildWindows.Size;
    ImGuiWindow* ed_win = (child_count > 0) ? parent_win->DC.ChildWindows.back() : nullptr;
    ImGuiInputTextState* post_state = ImGui::GetInputTextState(ed_id);
    const bool is_active = (post_state && ImGui::GetActiveID() == ed_id);
    const ImVec2 scroll = post_state ? post_state->Scroll : ImVec2(0.0f, 0.0f);

    if (is_active) {
        const char* dummy_start = nullptr;
        compute_cursor_line_col(css, post_state->GetCursorPos(), cur_line, cur_col, dummy_start);
    }

    parent_dl->PushClipRect(panel_p0, ImVec2(code_p0.x, code_p1.y), true);
    for (int ln = 1; ln <= line_count; ++ln) {
        char num_buf[16];
        std::snprintf(num_buf, sizeof(num_buf), "%d", ln);
        ImVec2 nsz = code_font->CalcTextSizeA(font_size, 200.0f, -1.0f, num_buf);
        float ny = panel_p0.y + pad_y + static_cast<float>(ln - 1) * line_h - scroll.y;
        ImU32 ncol = (ln == cur_line) ? ImGui::ColorConvertFloat4ToU32(c.accent) : IM_COL32(82, 96, 122, 255);
        parent_dl->AddText(code_font, font_size, ImVec2(code_p0.x - 10.0f - nsz.x, ny), ncol, num_buf);
    }
    parent_dl->PopClipRect();

    // Draw syntax-highlighted tokens and blinking cursor directly in child drawlist
    ImDrawList* ed_dl = ed_win ? ed_win->DrawList : parent_dl;
    const ImVec2 clip_min = ed_win ? ed_win->InnerClipRect.Min : ImVec2(code_p0.x + 1.0f, code_p0.y + 1.0f);
    const ImVec2 clip_max = ed_win ? ed_win->InnerClipRect.Max : ImVec2(code_p1.x - 1.0f, code_p1.y - 1.0f);
    ed_dl->PushClipRect(clip_min, clip_max, true);

    CssScanState scan;
    const char* text_begin = css.data();
    const char* text_end = text_begin + css.size();
    const char* line_start = text_begin;
    int ln_idx = 0;

    while (line_start <= text_end) {
        const char* nl = static_cast<const char*>(std::memchr(line_start, '\n', static_cast<size_t>(text_end - line_start)));
        const char* line_end = nl ? nl : text_end;
        float y = code_p0.y + pad_y + static_cast<float>(ln_idx) * line_h - scroll.y;
        if (y + line_h >= clip_min.y && y <= clip_max.y && line_end > line_start) {
            draw_highlighted_line(ed_dl, code_font, font_size, ImVec2(code_p0.x + pad_x - scroll.x, y),
                                  line_start, line_end, scan);
        } else {
            // Advance state across invisible lines if any
            CssScanState tmp = scan;
            for (const char* q = line_start; q < line_end; ++q) {
                if (tmp.in_comment && q + 1 < line_end && q[0] == '*' && q[1] == '/') {
                    tmp.in_comment = false;
                    ++q;
                } else if (!tmp.in_comment) {
                    if (q + 1 < line_end && q[0] == '/' && q[1] == '*') { tmp.in_comment = true; ++q; }
                    else if (*q == '{') { ++tmp.brace_depth; tmp.after_colon = false; }
                    else if (*q == '}') { tmp.brace_depth = std::max(0, tmp.brace_depth - 1); tmp.after_colon = false; }
                    else if (*q == ';' || *q == '\n') { tmp.after_colon = false; }
                    else if (*q == ':' && tmp.brace_depth > 0) { tmp.after_colon = true; }
                }
            }
            scan = tmp;
        }
        if (!nl) break;
        line_start = nl + 1;
        ++ln_idx;
    }

    if (is_active) {
        int c_ln = 1, c_col = 1;
        const char* c_line_start = text_begin;
        int c_pos = std::clamp(post_state->GetCursorPos(), 0, static_cast<int>(css.size()));
        compute_cursor_line_col(css, c_pos, c_ln, c_col, c_line_start);
        float cx = code_font->CalcTextSizeA(font_size, 100000.0f, -1.0f, c_line_start, text_begin + c_pos).x;
        float x = std::floor(code_p0.x + pad_x + cx - scroll.x);
        float y = std::floor(code_p0.y + pad_y + static_cast<float>(c_ln - 1) * line_h - scroll.y);
        if (std::fmod(post_state->CursorAnim, 1.20f) <= 0.80f) {
            ed_dl->AddLine(ImVec2(x, y + 1.0f), ImVec2(x, y + line_h - 1.0f), IM_COL32(255, 255, 255, 240), 1.5f);
        }
    }
    ed_dl->PopClipRect();

    // Footer status bar inside code panel
    const ImVec2 bar_p0(panel_p0.x, code_p1.y);
    const ImVec2 bar_p1 = panel_p1;
    parent_dl->AddRectFilled(bar_p0, bar_p1, IM_COL32(16, 21, 33, 255), 8.0f, ImDrawFlags_RoundCornersBottom);
    parent_dl->AddLine(bar_p0, ImVec2(bar_p1.x, bar_p0.y), IM_COL32(34, 42, 60, 255), 1.0f);
    parent_dl->AddRect(panel_p0, panel_p1, ImGui::ColorConvertFloat4ToU32(c.border), 8.0f);

    ImGui::PopFont();

    // Status bar text in small UI font
    ImGui::PushFont(ui.font_small);
    char left_meta[64];
    std::snprintf(left_meta, sizeof(left_meta), "CSS  |  UTF-8  |  %d %s", line_count, tr(lang, "obs_css_lines"));
    parent_dl->AddText(ui.font_small, ui.font_small->FontSize,
                       ImVec2(bar_p0.x + 12.0f, bar_p0.y + (status_h - ui.font_small->FontSize) * 0.5f),
                       IM_COL32(140, 155, 185, 255), left_meta);
    if (cur_line > 0) {
        char right_meta[48];
        std::snprintf(right_meta, sizeof(right_meta), "Ln %d, Col %d", cur_line, cur_col);
        ImVec2 rmsz = ui.font_small->CalcTextSizeA(ui.font_small->FontSize, 200.0f, -1.0f, right_meta);
        parent_dl->AddText(ui.font_small, ui.font_small->FontSize,
                           ImVec2(bar_p1.x - 12.0f - rmsz.x, bar_p0.y + (status_h - ui.font_small->FontSize) * 0.5f),
                           IM_COL32(190, 204, 232, 255), right_meta);
    }
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(avail_w, status_h));
    return changed;
}

} // namespace osmv::gui
