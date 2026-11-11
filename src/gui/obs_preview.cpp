#include "views.hpp"
#include "locale.hpp"
#include "common/obs_widget.hpp"
#include "IconsFontAwesome6.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace osmv::gui {

namespace {

ImVec4 hex_to_imvec4(const std::string& hex, ImVec4 fallback = ImVec4(0.06f, 0.06f, 0.06f, 0.88f)) {
    uint8_t r = 0, g = 0, b = 0, a = 255;
    if (!obs::parse_hex_rgba(hex, r, g, b, a)) return fallback;
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

ImVec4 hsl_to_imvec4(float h_deg, float s, float l, float a = 1.0f) {
    float h = std::fmod(std::fmod(h_deg, 360.0f) + 360.0f, 360.0f) / 360.0f;
    auto hue2rgb = [](float p, float q, float t) {
        if (t < 0.0f) t += 1.0f;
        if (t > 1.0f) t -= 1.0f;
        if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
        if (t < 1.0f / 2.0f) return q;
        if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
        return p;
    };
    if (s <= 0.0f) return ImVec4(l, l, l, a);
    float q = (l < 0.5f) ? (l * (1.0f + s)) : (l + s - l * s);
    float p = 2.0f * l - q;
    return ImVec4(hue2rgb(p, q, h + 1.0f / 3.0f), hue2rgb(p, q, h), hue2rgb(p, q, h - 1.0f / 3.0f), a);
}

float hue_from_imvec4(const ImVec4& col) {
    float mx = std::max({col.x, col.y, col.z});
    float mn = std::min({col.x, col.y, col.z});
    if (mx == mn) return 260.0f;
    float d = mx - mn;
    float h = 0.0f;
    if (mx == col.x) h = ((col.y - col.z) / d + (col.y < col.z ? 6.0f : 0.0f)) / 6.0f;
    else if (mx == col.y) h = ((col.z - col.x) / d + 2.0f) / 6.0f;
    else h = ((col.x - col.y) / d + 4.0f) / 6.0f;
    return h * 360.0f;
}

ImU32 to_u32(const ImVec4& col, float alpha_mult = 1.0f) {
    ImVec4 c = col;
    c.w = std::clamp(c.w * alpha_mult, 0.0f, 1.0f);
    return ImGui::ColorConvertFloat4ToU32(c);
}

void draw_checkerboard(ImDrawList* dl, ImVec2 p0, ImVec2 p1, int mode) {
    if (mode == 1) {
        dl->AddRectFilled(p0, p1, IM_COL32(13, 16, 24, 255), 10.0f);
        return;
    }
    if (mode == 2) {
        dl->AddRectFilled(p0, p1, IM_COL32(226, 232, 242, 255), 10.0f);
        return;
    }
    dl->PushClipRect(p0, p1, true);
    const float tile = 14.0f;
    const ImU32 c1 = IM_COL32(24, 27, 38, 255);
    const ImU32 c2 = IM_COL32(32, 36, 50, 255);
    dl->AddRectFilled(p0, p1, c1);
    for (float y = p0.y; y < p1.y; y += tile) {
        int row = static_cast<int>((y - p0.y) / tile);
        for (float x = p0.x; x < p1.x; x += tile) {
            int col = static_cast<int>((x - p0.x) / tile);
            if ((row + col) % 2 == 0) {
                dl->AddRectFilled(ImVec2(x, y), ImVec2(std::min(x + tile, p1.x), std::min(y + tile, p1.y)), c2);
            }
        }
    }
    dl->PopClipRect();
}

float eq_sample(int idx, int count, float t, float speed) {
    float norm = (count <= 1) ? 0.5f : static_cast<float>(idx) / static_cast<float>(count - 1);
    float center_bias = 1.0f - std::abs(norm - 0.5f) * 0.7f;
    float s1 = std::sin(t * 4.2f * speed + idx * 0.75f);
    float s2 = std::sin(t * 7.1f * speed - idx * 1.15f);
    float s3 = std::cos(t * 2.5f * speed + idx * 0.42f);
    float v = 0.42f + 0.25f * s1 + 0.18f * s2 + 0.15f * s3;
    return std::clamp(v * center_bias, 0.08f, 1.0f);
}

ImU32 viz_bar_color(const ObsWidgetSettings& w, int idx, int count, float hue, float t, float y_norm, float alpha_mult) {
    if (w.viz_color_mode == "rainbow") {
        float rh = std::fmod(idx * (360.0f / std::max(1, count)) + t * 35.0f, 360.0f);
        return to_u32(hsl_to_imvec4(rh, 0.88f, 0.62f - 0.18f * y_norm, 0.94f), alpha_mult);
    }
    if (w.viz_color_mode == "custom") {
        ImVec4 top_c = hex_to_imvec4(w.viz_color_top);
        ImVec4 bot_c = hex_to_imvec4(w.viz_color_bottom);
        ImVec4 mix(
            top_c.x + (bot_c.x - top_c.x) * y_norm,
            top_c.y + (bot_c.y - top_c.y) * y_norm,
            top_c.z + (bot_c.z - top_c.z) * y_norm,
            top_c.w + (bot_c.w - top_c.w) * y_norm);
        return to_u32(mix, alpha_mult);
    }
    return to_u32(hsl_to_imvec4(hue, 0.82f, 0.68f - 0.24f * y_norm, 0.94f), alpha_mult);
}

void draw_visualizer_preview(ImDrawList* dl,
                             ImVec2 p0,
                             ImVec2 p1,
                             const ObsWidgetSettings& w,
                             float hue,
                             float t_sec,
                             float alpha_mult) {
    float width = p1.x - p0.x;
    float height = p1.y - p0.y;
    if (width <= 4.0f || height <= 4.0f) return;

    int count = std::clamp(w.viz_bar_count, 6, 48);
    float gap = std::max(1.0f, static_cast<float>(w.viz_gap_px) * 0.7f);
    float bar_w = std::max(2.0f, (width - gap * (count - 1)) / count);

    if (w.viz_style == "wave") {
        ImVec2 prev{};
        for (int i = 0; i < count; ++i) {
            float amp = eq_sample(i, count, t_sec, w.viz_speed);
            float x = (count <= 1) ? p0.x : p0.x + (static_cast<float>(i) / (count - 1)) * width;
            float y = p1.y - amp * (height * 0.85f) - 2.0f;
            ImVec2 cur(x, y);
            if (i > 0) {
                ImU32 fill_c = viz_bar_color(w, i, count, hue, t_sec, 0.5f, 0.30f * alpha_mult);
                ImVec2 q1(prev.x, p1.y);
                ImVec2 q2(cur.x, p1.y);
                dl->AddQuadFilled(prev, cur, q2, q1, fill_c);
                ImU32 line_c = viz_bar_color(w, i, count, hue, t_sec, 0.1f, alpha_mult);
                dl->AddLine(prev, cur, line_c, 2.2f);
            }
            prev = cur;
        }
        return;
    }

    for (int i = 0; i < count; ++i) {
        float amp = eq_sample(i, count, t_sec, w.viz_speed);
        float x = p0.x + i * (bar_w + gap);

        if (w.viz_style == "mirrored") {
            float bh = std::max(3.0f, amp * (height - 2.0f));
            float cy = (p0.y + p1.y) * 0.5f;
            ImU32 col = viz_bar_color(w, i, count, hue, t_sec, 0.25f, alpha_mult);
            dl->AddRectFilled(ImVec2(x, cy - bh * 0.5f), ImVec2(x + bar_w, cy + bh * 0.5f),
                              col, std::min(bar_w * 0.5f, 3.0f));
        } else if (w.viz_style == "dots") {
            float cy = p1.y - std::max(3.0f, amp * (height - 4.0f));
            float r = std::clamp(bar_w * 0.45f, 2.0f, 5.5f);
            ImU32 stem_c = viz_bar_color(w, i, count, hue, t_sec, 0.7f, 0.26f * alpha_mult);
            dl->AddRectFilled(ImVec2(x + bar_w * 0.5f - 0.8f, cy), ImVec2(x + bar_w * 0.5f + 0.8f, p1.y), stem_c);
            ImU32 dot_c = viz_bar_color(w, i, count, hue, t_sec, 0.1f, alpha_mult);
            dl->AddCircleFilled(ImVec2(x + bar_w * 0.5f, cy), r, dot_c);
        } else if (w.viz_style == "blocks") {
            float bh = std::max(4.0f, amp * height);
            float blk = 3.5f;
            float step = blk + 2.0f;
            int nblk = std::max(1, static_cast<int>(bh / step));
            for (int b = 0; b < nblk; ++b) {
                float by2 = p1.y - b * step;
                float by1 = by2 - blk;
                if (by1 < p0.y) break;
                float yn = 1.0f - (by1 - p0.y) / std::max(1.0f, height);
                dl->AddRectFilled(ImVec2(x, by1), ImVec2(x + bar_w, by2),
                                  viz_bar_color(w, i, count, hue, t_sec, 1.0f - yn, alpha_mult), 1.2f);
            }
        } else {
            float bh = std::max(3.0f, amp * height);
            ImU32 top_c = viz_bar_color(w, i, count, hue, t_sec, 0.0f, alpha_mult);
            ImU32 bot_c = viz_bar_color(w, i, count, hue, t_sec, 1.0f, alpha_mult);
            dl->AddRectFilledMultiColor(ImVec2(x, p1.y - bh), ImVec2(x + bar_w, p1.y),
                                        top_c, top_c, bot_c, bot_c);
        }
    }
}

void draw_artwork(ImDrawList* dl,
                  ImTextureID tex,
                  ImVec2 p0,
                  ImVec2 p1,
                  float radius,
                  float spin_angle,
                  bool is_vinyl,
                  ImFont* icon_font) {
    float sz = p1.x - p0.x;
    float r = std::clamp(radius, 0.0f, sz * 0.5f);
    if (tex) {
        dl->PushTextureID(tex);
        int vtx_start = dl->VtxBuffer.Size;
        dl->PathRect(p0, p1, r, ImDrawFlags_RoundCornersAll);
        dl->PathFillConvex(IM_COL32_WHITE);
        int vtx_end = dl->VtxBuffer.Size;

        float cs = std::cos(spin_angle);
        float sn = std::sin(spin_angle);
        ImVec2 inv_size(1.0f / sz, 1.0f / sz);
        for (int i = vtx_start; i < vtx_end; ++i) {
            ImVec2 uv((dl->VtxBuffer[i].pos.x - p0.x) * inv_size.x - 0.5f,
                      (dl->VtxBuffer[i].pos.y - p0.y) * inv_size.y - 0.5f);
            dl->VtxBuffer[i].uv.x = 0.5f + uv.x * cs - uv.y * sn;
            dl->VtxBuffer[i].uv.y = 0.5f + uv.x * sn + uv.y * cs;
        }
        dl->PopTextureID();
    } else {
        dl->AddRectFilled(p0, p1, IM_COL32(40, 44, 62, 220), r);
        dl->AddRect(p0, p1, IM_COL32(255, 255, 255, 40), r);
        if (icon_font) {
            ImVec2 isz = icon_font->CalcTextSizeA(sz * 0.38f, 200.0f, -1.0f, ICON_FA_MUSIC);
            dl->AddText(icon_font, sz * 0.38f,
                        ImVec2(p0.x + (sz - isz.x) * 0.5f, p0.y + (sz - isz.y) * 0.5f),
                        IM_COL32(220, 226, 245, 160), ICON_FA_MUSIC);
        }
    }

    if (is_vinyl) {
        ImVec2 c((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f);
        float hole_r = std::max(3.5f, sz * 0.11f);
        dl->AddCircleFilled(c, hole_r + 2.0f, IM_COL32(255, 255, 255, 185));
        dl->AddCircleFilled(c, hole_r, IM_COL32(18, 20, 28, 255));
    }
}

void draw_aligned_text(ImDrawList* dl,
                       ImFont* font,
                       float font_size,
                       ImVec2 box_p0,
                       float box_w,
                       float y,
                       const std::string& text,
                       ImU32 col,
                       const std::string& align,
                       bool shadow,
                       bool marquee,
                       float scroll_speed,
                       double time_sec) {
    if (text.empty() || !font) return;
    ImVec2 tsz = font->CalcTextSizeA(font_size, 10000.0f, -1.0f, text.c_str());
    float x = box_p0.x;
    if (tsz.x > box_w && marquee) {
        float overflow = tsz.x - box_w;
        float cycle = std::max(4.0f, 6.0f / std::max(0.4f, scroll_speed));
        float phase = static_cast<float>(std::fmod(time_sec, cycle * 2.0f));
        float norm = (phase <= cycle) ? (phase / cycle) : (2.0f - phase / cycle);
        float smooth = norm * norm * (3.0f - 2.0f * norm);
        x -= overflow * smooth;
    } else if (align == "center") {
        x = box_p0.x + std::max(0.0f, (box_w - tsz.x) * 0.5f);
    } else if (align == "right") {
        x = box_p0.x + std::max(0.0f, box_w - tsz.x);
    }

    dl->PushClipRect(ImVec2(box_p0.x, y - 2.0f), ImVec2(box_p0.x + box_w, y + font_size + 6.0f), true);
    if (shadow) {
        dl->AddText(font, font_size, ImVec2(x + 1.0f, y + 1.5f), IM_COL32(0, 0, 0, 150), text.c_str());
    }
    dl->AddText(font, font_size, ImVec2(x, y), col, text.c_str());
    dl->PopClipRect();
}

} // namespace

void render_obs_widget_preview(GuiAppState& st) {
    const auto& lang = st.settings.general.language;
    auto& ui = st.ui;
    const auto& c = ui.colors;
    const auto& w = st.settings.obs_widget;

    // Toolbar above preview canvas
    section_title(ui, tr(lang, "obs_preview_title"));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(9, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

    if (st.obs_preview_demo ? subtle_button(c, tr(lang, "obs_mode_live")) : accent_button(c, tr(lang, "obs_mode_live"))) {
        st.obs_preview_demo = false;
    }
    ImGui::SameLine(0, 6);
    if (st.obs_preview_demo ? accent_button(c, tr(lang, "obs_mode_demo")) : subtle_button(c, tr(lang, "obs_mode_demo"))) {
        st.obs_preview_demo = true;
    }
    ImGui::SameLine(0, 14);
    const char* bg_labels[3] = {tr(lang, "obs_bg_checker"), tr(lang, "obs_bg_dark"), tr(lang, "obs_bg_light")};
    for (int i = 0; i < 3; ++i) {
        if (i > 0) ImGui::SameLine(0, 4);
        bool sel = (st.obs_preview_backdrop == i);
        if (sel ? accent_button(c, bg_labels[i]) : subtle_button(c, bg_labels[i])) {
            st.obs_preview_backdrop = i;
        }
    }
    ImGui::PopStyleVar(2);

    ImGui::Dummy(ImVec2(0, 4));

    const float avail_w = std::max(320.0f, ImGui::GetContentRegionAvail().x);
    const float stage_h = (w.layout == "vertical") ? 260.0f : 206.0f;
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1(p0.x + avail_w, p0.y + stage_h);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    draw_checkerboard(dl, p0, p1, st.obs_preview_backdrop);
    dl->AddRect(p0, p1, to_u32(c.border), 10.0f);
    ImGui::Dummy(ImVec2(avail_w, stage_h));

    // Resolve live vs demo track
    const bool has_live = (!st.snap.title.empty() && st.snap.status != "closed" && st.snap.status != "stopped");
    const bool use_demo = st.obs_preview_demo || !has_live;
    const std::string song = use_demo ? "Midnight City" : st.snap.title;
    const std::string artist = use_demo ? "M83" : st.snap.artist;
    const std::string album = use_demo ? "Hurry Up, We're Dreaming" : st.snap.album;
    const std::string player = use_demo ? "Spotify" : st.snap.player_name;
    const std::string status = use_demo ? "playing" : st.snap.status;
    ImTextureID art_tex = use_demo ? st.demo_album_tex : st.album_tex;
    const bool active_play = use_demo || (st.snap.status == "playing");

    // Trigger 60 FPS only when animated elements are visible
    const double t_now = ImGui::GetTime();
    if ((st.settings.audio_visualizer && active_play) || (w.spin_artwork && active_play) || w.marquee_scroll) {
        ui.animating = true;
    }

    // Scaled preview geometry
    const float scale = std::min(1.0f, (avail_w - 36.0f) / static_cast<float>(std::clamp(w.width_px, 260, 800)));
    const float card_w = w.width_px * scale;
    const float pad = w.padding_px * scale;
    const float gap = w.gap_px * scale;
    const float art_sz = w.show_album_art ? (w.art_size_px * scale) : 0.0f;
    const bool is_vertical = (w.layout == "vertical");

    const std::string badge_str = w.show_status_badge ? obs::format_widget_text(w.badge_format, song, artist, album, player, status) : "";
    const std::string title_str = w.show_title ? obs::format_widget_text(w.title_format, song, artist, album, player, status) : "";
    const std::string artist_str = w.show_artist ? obs::format_widget_text(w.artist_format, song, artist, album, player, status) : "";
    const std::string album_str = (w.show_album && !album.empty()) ? obs::format_widget_text(w.album_format, song, artist, album, player, status) : "";

    const float t_sz = std::max(10.0f, w.title_size_px * scale);
    const float a_sz = std::max(9.0f, w.artist_size_px * scale);
    const float al_sz = std::max(8.0f, w.album_size_px * scale);
    float text_block_h = 0.0f;
    if (!badge_str.empty()) text_block_h += 18.0f * scale + 4.0f;
    if (!title_str.empty()) text_block_h += t_sz + 4.0f;
    if (!artist_str.empty()) text_block_h += a_sz + 3.0f;
    if (!album_str.empty()) text_block_h += al_sz + 2.0f;
    text_block_h = std::max(text_block_h, 20.0f);

    const float content_h = is_vertical ? (art_sz + (w.show_album_art ? gap : 0.0f) + text_block_h) : std::max(art_sz, text_block_h);
    const float card_h = content_h + pad * 2.0f;

    const bool show_outer_viz = st.settings.audio_visualizer && (w.viz_position == "top" || w.viz_position == "bottom");
    const float viz_h = show_outer_viz ? std::clamp(w.viz_height_px * scale, 18.0f, 70.0f) : 0.0f;
    const float stack_gap = show_outer_viz ? 7.0f * scale : 0.0f;
    const float total_h = card_h + viz_h + stack_gap;

    const float sx = p0.x + (avail_w - card_w) * 0.5f;
    const float sy = p0.y + std::max(12.0f, (stage_h - total_h) * 0.5f);

    float card_y = sy;
    if (show_outer_viz && w.viz_position == "top") {
        card_y = sy + viz_h + stack_gap;
    }

    ImVec2 card_p0(sx, card_y);
    ImVec2 card_p1(sx + card_w, card_y + card_h);
    const float card_rad = std::min(card_h * 0.5f, static_cast<float>(w.border_radius_px) * scale);

    // Resolve colors + Dynamic Album Color
    ImVec4 bg1 = hex_to_imvec4(w.bg_color);
    ImVec4 bg2 = w.use_bg_gradient ? hex_to_imvec4(w.bg_color_2) : bg1;
    ImVec4 brd_c = hex_to_imvec4(w.border_color);
    ImVec4 acc_c = hex_to_imvec4(w.accent_color);

    float active_hue = use_demo ? 268.0f : (st.has_album_color ? st.album_hue : hue_from_imvec4(acc_c));
    if (st.settings.dynamic_color) {
        if (w.dynamic_color_mode == "accent") {
            brd_c = hsl_to_imvec4(active_hue, 0.82f, 0.58f, 0.95f);
            acc_c = hsl_to_imvec4(active_hue, 0.78f, 0.50f, 1.0f);
        } else if (w.dynamic_color_mode == "subtle") {
            bg1 = hsl_to_imvec4(active_hue, 0.65f, 0.22f, 0.88f);
            bg2 = ImVec4(0.06f, 0.07f, 0.10f, 0.90f);
            brd_c = hsl_to_imvec4(active_hue, 0.75f, 0.55f, 0.55f);
            acc_c = hsl_to_imvec4(active_hue, 0.72f, 0.48f, 1.0f);
        } else {
            bg1 = hsl_to_imvec4(active_hue, 0.72f, 0.20f, 0.94f);
            bg2 = bg1;
            brd_c = hsl_to_imvec4(active_hue, 0.72f, 0.48f, 1.0f);
            acc_c = hsl_to_imvec4(active_hue, 0.80f, 0.52f, 1.0f);
        }
    } else {
        active_hue = hue_from_imvec4(acc_c);
    }

    dl->PushClipRect(p0, p1, true);

    // Outer glow / shadow approximation
    if (w.glow_enabled || (st.settings.dynamic_color && w.shadow_intensity > 0.05f)) {
        ImVec4 glow_c = st.settings.dynamic_color ? hsl_to_imvec4(active_hue, 0.85f, 0.52f, 0.22f)
                                                  : ImVec4(acc_c.x, acc_c.y, acc_c.z, 0.22f);
        dl->AddRect(ImVec2(card_p0.x - 2.0f, card_p0.y - 2.0f), ImVec2(card_p1.x + 2.0f, card_p1.y + 2.0f),
                    to_u32(glow_c), card_rad + 2.0f, 0, 4.0f);
    }
    if (w.shadow_intensity > 0.05f) {
        dl->AddRectFilled(ImVec2(card_p0.x + 2.0f, card_p0.y + 5.0f), ImVec2(card_p1.x + 2.0f, card_p1.y + 6.0f),
                          IM_COL32(0, 0, 0, static_cast<int>(w.shadow_intensity * 110.0f)), card_rad);
    }

    // Outer visualizer (top / bottom)
    if (show_outer_viz) {
        float vy = (w.viz_position == "top") ? sy : (card_p1.y + stack_gap);
        draw_visualizer_preview(dl, ImVec2(sx, vy), ImVec2(sx + card_w, vy + viz_h),
                                w, active_hue, static_cast<float>(t_now), 1.0f);
    }

    // Card fill (solid / rounded gradient)
    if (bg1.w > 0.01f || bg2.w > 0.01f) {
        if (w.use_bg_gradient || (st.settings.dynamic_color && w.dynamic_color_mode == "subtle")) {
            int v_start = dl->VtxBuffer.Size;
            dl->PathRect(card_p0, card_p1, card_rad, ImDrawFlags_RoundCornersAll);
            dl->PathFillConvex(to_u32(bg1));
            int v_end = dl->VtxBuffer.Size;
            float inv_w = 1.0f / std::max(1.0f, card_w);
            for (int vi = v_start; vi < v_end; ++vi) {
                float t = std::clamp((dl->VtxBuffer[vi].pos.x - card_p0.x) * inv_w, 0.0f, 1.0f);
                ImVec4 mc(bg1.x + (bg2.x - bg1.x) * t, bg1.y + (bg2.y - bg1.y) * t,
                          bg1.z + (bg2.z - bg1.z) * t, bg1.w + (bg2.w - bg1.w) * t);
                dl->VtxBuffer[vi].col = to_u32(mc);
            }
        } else {
            dl->AddRectFilled(card_p0, card_p1, to_u32(bg1), card_rad);
        }
    }

    // Background visualizer clipped inside card
    if (st.settings.audio_visualizer && w.viz_position == "background") {
        dl->PushClipRect(card_p0, card_p1, true);
        draw_visualizer_preview(dl, card_p0, card_p1, w, active_hue, static_cast<float>(t_now), 0.30f);
        dl->PopClipRect();
    }

    if (w.border_width_px > 0 && brd_c.w > 0.01f) {
        dl->AddRect(card_p0, card_p1, to_u32(brd_c), card_rad, 0, std::max(1.0f, w.border_width_px * scale));
    }

    // Content layout inside card
    float art_x = card_p0.x + pad;
    float art_y = card_p0.y + (card_h - art_sz) * 0.5f;
    float info_x = card_p0.x + pad;
    float info_w = card_w - pad * 2.0f;
    float info_y = card_p0.y + (card_h - text_block_h) * 0.5f;

    if (w.show_album_art) {
        if (is_vertical) {
            art_x = card_p0.x + (card_w - art_sz) * 0.5f;
            art_y = card_p0.y + pad;
            info_x = card_p0.x + pad;
            info_w = card_w - pad * 2.0f;
            info_y = art_y + art_sz + gap;
        } else if (w.layout == "horizontal-right") {
            art_x = card_p1.x - pad - art_sz;
            info_x = card_p0.x + pad;
            info_w = card_w - pad * 2.0f - art_sz - gap;
        } else {
            art_x = card_p0.x + pad;
            info_x = art_x + art_sz + gap;
            info_w = card_p1.x - pad - info_x;
        }

        float art_rad = std::min(art_sz * 0.5f, static_cast<float>(w.art_radius_px) * scale);
        bool is_vinyl = w.spin_artwork && (art_rad >= art_sz * 0.42f);
        float spin = (w.spin_artwork && active_play) ? static_cast<float>(std::fmod(t_now * 1.05, 6.2831853)) : 0.0f;
        draw_artwork(dl, art_tex, ImVec2(art_x, art_y), ImVec2(art_x + art_sz, art_y + art_sz),
                     art_rad, spin, is_vinyl, ui.font_title);
    }

    std::string effective_align = is_vertical ? "center" : w.text_align;
    float cur_y = info_y;

    if (!badge_str.empty() && ui.font_small) {
        float bf_sz = std::max(8.5f, 10.5f * scale);
        ImVec2 bsz = ui.font_small->CalcTextSizeA(bf_sz, 300.0f, -1.0f, badge_str.c_str());
        float pill_w = std::min(info_w, bsz.x + 12.0f);
        float pill_h = bf_sz + 5.0f;
        float bx = info_x;
        if (effective_align == "center") bx = info_x + (info_w - pill_w) * 0.5f;
        else if (effective_align == "right") bx = info_x + info_w - pill_w;
        dl->AddRectFilled(ImVec2(bx, cur_y), ImVec2(bx + pill_w, cur_y + pill_h), to_u32(acc_c), pill_h * 0.5f);
        dl->PushClipRect(ImVec2(bx, cur_y), ImVec2(bx + pill_w, cur_y + pill_h), true);
        dl->AddText(ui.font_small, bf_sz, ImVec2(bx + 6.0f, cur_y + 2.0f), IM_COL32(255, 255, 255, 255), badge_str.c_str());
        dl->PopClipRect();
        cur_y += pill_h + 4.0f;
    }

    if (!title_str.empty()) {
        draw_aligned_text(dl, ui.font_title ? ui.font_title : ui.font_regular, t_sz,
                          ImVec2(info_x, cur_y), info_w, cur_y, title_str,
                          to_u32(hex_to_imvec4(w.title_color)), effective_align,
                          w.text_shadow, w.marquee_scroll, w.scroll_speed, t_now);
        cur_y += t_sz + 4.0f;
    }
    if (!artist_str.empty()) {
        draw_aligned_text(dl, ui.font_regular, a_sz,
                          ImVec2(info_x, cur_y), info_w, cur_y, artist_str,
                          to_u32(hex_to_imvec4(w.artist_color)), effective_align,
                          w.text_shadow, w.marquee_scroll, w.scroll_speed, t_now);
        cur_y += a_sz + 3.0f;
    }
    if (!album_str.empty()) {
        draw_aligned_text(dl, ui.font_small ? ui.font_small : ui.font_regular, al_sz,
                          ImVec2(info_x, cur_y), info_w, cur_y, album_str,
                          to_u32(hex_to_imvec4(w.album_color)), effective_align,
                          w.text_shadow, w.marquee_scroll, w.scroll_speed, t_now);
    }

    // Subtle status indicators in corner of stage
    char info_pill[96];
    std::snprintf(info_pill, sizeof(info_pill), "%s  |  %dpx  |  %s",
                  use_demo ? tr(lang, "obs_mode_demo") : tr(lang, "obs_mode_live"),
                  w.width_px, w.position.c_str());
    if (ui.font_small) {
        dl->AddText(ui.font_small, ui.font_small->FontSize, ImVec2(p0.x + 10.0f, p0.y + 8.0f),
                    (st.obs_preview_backdrop == 2) ? IM_COL32(30, 35, 50, 210) : IM_COL32(220, 226, 245, 200),
                    info_pill);
        if (w.custom_css_enabled) {
            const char* css_lbl = tr(lang, "obs_css_active_pill");
            ImVec2 csz = ui.font_small->CalcTextSizeA(ui.font_small->FontSize, 600.0f, -1.0f, css_lbl);
            dl->AddText(ui.font_small, ui.font_small->FontSize, ImVec2(p1.x - csz.x - 10.0f, p1.y - csz.y - 6.0f),
                        to_u32(c.accent), css_lbl);
        }
    }

    dl->PopClipRect();
}

} // namespace osmv::gui
