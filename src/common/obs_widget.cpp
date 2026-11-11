#include "obs_widget.hpp"
#include "formatter.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>

namespace osmv::obs {

namespace {
int hex_val(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

std::string to_upper_ascii(std::string s) {
    for (char& ch : s) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return s;
}

std::string replace_token(std::string src, const std::string& from, const std::string& to) {
    if (from.empty()) return src;
    size_t pos = 0;
    while ((pos = src.find(from, pos)) != std::string::npos) {
        src.replace(pos, from.size(), to);
        pos += to.size();
    }
    return src;
}
} // namespace

const std::vector<PresetInfo>& preset_catalog() {
    static const std::vector<PresetInfo> kPresets = {
        {"classic", "Classic Dark", "Classique Sombre",
         "Dark card with subtle border and top equalizer",
         "Carte sombre classique avec bordure fine et visualiseur"},
        {"glass", "Frosted Glass", "Verre Depoli",
         "Translucent blurred card with status pill and wave",
         "Carte translucide avec flou d'arriere-plan et onde"},
        {"neon", "Neon Cyber", "Neon Cyber",
         "Vibrant cyber border, outer glow, and segmented LED bars",
         "Bordure neon, halo lumineux et barres LED segmentees"},
        {"compact", "Compact Pill", "Pilule Compacte",
         "Streamlined capsule layout with circular artwork",
         "Capsule compacte avec pochette circulaire"},
        {"vinyl", "Vinyl Studio", "Studio Vinyle",
         "Rotating circular vinyl cover and mirrored warm equalizer",
         "Pochette vinyle rotative et visualiseur symetrique chaud"},
        {"minimal", "Minimal Clean", "Minimaliste",
         "Transparent card with crisp drop-shadow typography",
         "Fond transparent et typographie nette avec ombre portee"},
        {"banner", "Lower-Third", "Bandeau Large",
         "Wide broadcast bar with background waveform",
         "Bandeau horizontal large avec visualiseur en arriere-plan"},
    };
    return kPresets;
}

bool apply_preset(ObsWidgetSettings& w, const std::string& preset_id) {
    // Preserve custom CSS and user position/idle choices only? Reset visual & content layout coherently,
    // while preserving user's hand-written custom_css buffer so switching presets never destroys their code.
    std::string saved_css = w.custom_css;
    bool saved_css_en = w.custom_css_enabled;
    bool saved_css_raw = w.custom_css_raw_override;
    std::string saved_pos = w.position;
    int saved_margin = w.margin_px;

    w = ObsWidgetSettings{};
    w.position = saved_pos;
    w.margin_px = saved_margin;
    w.custom_css = std::move(saved_css);
    w.custom_css_enabled = saved_css_en;
    w.custom_css_raw_override = saved_css_raw;

    if (preset_id == "classic") {
        w.preset = "classic";
        return true;
    }
    if (preset_id == "glass") {
        w.preset = "glass";
        w.width_px = 460;
        w.padding_px = 18;
        w.border_radius_px = 20;
        w.art_size_px = 82;
        w.art_radius_px = 12;
        w.show_album = true;
        w.show_status_badge = true;
        w.badge_format = "$status_upper";
        w.bg_color = "#1216299E";
        w.use_bg_gradient = true;
        w.bg_color_2 = "#1E294BA8";
        w.backdrop_blur_px = 18;
        w.border_color = "#FFFFFF38";
        w.border_width_px = 1;
        w.accent_color = "#58A6FFFF";
        w.title_color = "#FFFFFFFF";
        w.artist_color = "#D6E2FFFF";
        w.album_color = "#9BB0D9FF";
        w.shadow_intensity = 0.55f;
        w.dynamic_color_mode = "subtle";
        w.title_weight = 700;
        w.viz_position = "top";
        w.viz_style = "wave";
        w.viz_bar_count = 28;
        w.viz_height_px = 48;
        w.viz_color_mode = "album";
        w.transition_style = "slide-up";
        return true;
    }
    if (preset_id == "neon") {
        w.preset = "neon";
        w.width_px = 460;
        w.padding_px = 18;
        w.border_radius_px = 10;
        w.art_size_px = 78;
        w.art_radius_px = 6;
        w.show_status_badge = true;
        w.badge_format = "LIVE | $player";
        w.bg_color = "#090B16E8";
        w.use_bg_gradient = true;
        w.bg_color_2 = "#1A0D2EE8";
        w.backdrop_blur_px = 8;
        w.border_color = "#00E5FFCC";
        w.border_width_px = 2;
        w.accent_color = "#FF2E93FF";
        w.title_color = "#FFFFFFFF";
        w.artist_color = "#7CE8FFFF";
        w.album_color = "#FF85C2FF";
        w.shadow_intensity = 0.70f;
        w.glow_enabled = true;
        w.dynamic_color_mode = "accent";
        w.title_size_px = 19;
        w.title_weight = 700;
        w.viz_position = "bottom";
        w.viz_style = "blocks";
        w.viz_bar_count = 24;
        w.viz_height_px = 44;
        w.viz_color_mode = "custom";
        w.viz_color_top = "#00E5FFFF";
        w.viz_color_bottom = "#FF2E93FF";
        w.transition_style = "zoom";
        return true;
    }
    if (preset_id == "compact") {
        w.preset = "compact";
        w.layout = "compact";
        w.width_px = 380;
        w.padding_px = 10;
        w.gap_px = 12;
        w.border_radius_px = 999;
        w.art_size_px = 52;
        w.art_radius_px = 26;
        w.bg_color = "#10121AD9";
        w.backdrop_blur_px = 12;
        w.border_color = "#FFFFFF26";
        w.border_width_px = 1;
        w.accent_color = "#34D399FF";
        w.title_color = "#FFFFFFFF";
        w.artist_color = "#B7C4D6FF";
        w.shadow_intensity = 0.35f;
        w.dynamic_color_mode = "accent";
        w.title_size_px = 15;
        w.artist_size_px = 12;
        w.text_shadow = false;
        w.viz_position = "background";
        w.viz_style = "bars";
        w.viz_bar_count = 28;
        w.viz_height_px = 40;
        w.viz_gap_px = 3;
        return true;
    }
    if (preset_id == "vinyl") {
        w.preset = "vinyl";
        w.width_px = 440;
        w.padding_px = 18;
        w.gap_px = 18;
        w.border_radius_px = 22;
        w.art_size_px = 86;
        w.art_radius_px = 43;
        w.spin_artwork = true;
        w.show_album = true;
        w.show_status_badge = true;
        w.badge_format = "SPINNING";
        w.bg_color = "#181312EC";
        w.use_bg_gradient = true;
        w.bg_color_2 = "#261B18EC";
        w.backdrop_blur_px = 6;
        w.border_color = "#F59E0B55";
        w.border_width_px = 1;
        w.accent_color = "#F59E0BFF";
        w.title_color = "#FFF8EBFF";
        w.artist_color = "#E5C69BFF";
        w.album_color = "#B89A72FF";
        w.shadow_intensity = 0.55f;
        w.dynamic_color_mode = "subtle";
        w.title_weight = 700;
        w.viz_position = "bottom";
        w.viz_style = "mirrored";
        w.viz_bar_count = 24;
        w.viz_height_px = 42;
        w.viz_color_mode = "custom";
        w.viz_color_top = "#FBBF24FF";
        w.viz_color_bottom = "#EA580CFF";
        return true;
    }
    if (preset_id == "minimal") {
        w.preset = "minimal";
        w.width_px = 420;
        w.padding_px = 8;
        w.gap_px = 14;
        w.border_radius_px = 12;
        w.art_size_px = 70;
        w.art_radius_px = 12;
        w.bg_color = "#00000000";
        w.border_color = "#FFFFFF00";
        w.border_width_px = 0;
        w.accent_color = "#FFFFFFFF";
        w.title_color = "#FFFFFFFF";
        w.artist_color = "#E2E8F0FF";
        w.shadow_intensity = 0.0f;
        w.dynamic_color_mode = "accent";
        w.title_size_px = 20;
        w.title_weight = 700;
        w.viz_position = "top";
        w.viz_style = "dots";
        w.viz_bar_count = 22;
        w.viz_height_px = 40;
        w.viz_gap_px = 5;
        return true;
    }
    if (preset_id == "banner") {
        w.preset = "banner";
        w.width_px = 620;
        w.padding_px = 14;
        w.gap_px = 14;
        w.border_radius_px = 10;
        w.art_size_px = 58;
        w.art_radius_px = 8;
        w.show_album = true;
        w.show_status_badge = true;
        w.badge_format = "$status_upper";
        w.bg_color = "#0B0F19EB";
        w.backdrop_blur_px = 10;
        w.border_color = "#6366F1FF";
        w.border_width_px = 2;
        w.accent_color = "#6366F1FF";
        w.title_color = "#FFFFFFFF";
        w.artist_color = "#CBD5E1FF";
        w.album_color = "#94A3B8FF";
        w.shadow_intensity = 0.50f;
        w.glow_enabled = true;
        w.dynamic_color_mode = "accent";
        w.title_size_px = 17;
        w.artist_size_px = 13;
        w.album_size_px = 11;
        w.title_weight = 700;
        w.text_shadow = false;
        w.viz_position = "background";
        w.viz_style = "mirrored";
        w.viz_bar_count = 36;
        w.viz_height_px = 52;
        w.viz_gap_px = 3;
        w.transition_style = "slide-left";
        return true;
    }
    return false;
}

void reset_widget_defaults(AppSettings& s) {
    s.obs_widget = ObsWidgetSettings{};
    s.dynamic_color = false;
    s.audio_visualizer = false;
}

bool upsert_custom_preset(AppSettings& s, const std::string& name) {
    if (name.empty()) return false;
    ObsWidgetSettings w = s.obs_widget;
    w.preset = "user:" + name;
    s.obs_widget.preset = w.preset;

    for (auto& p : s.obs_custom_presets) {
        if (p.name == name) {
            p.dynamic_color = s.dynamic_color;
            p.audio_visualizer = s.audio_visualizer;
            p.widget = std::move(w);
            return true;
        }
    }
    s.obs_custom_presets.push_back(ObsCustomPreset{
        name, s.dynamic_color, s.audio_visualizer, std::move(w)
    });
    return true;
}

bool apply_custom_preset(AppSettings& s, const std::string& name) {
    for (const auto& p : s.obs_custom_presets) {
        if (p.name == name) {
            s.obs_widget = p.widget;
            s.obs_widget.preset = "user:" + p.name;
            s.dynamic_color = p.dynamic_color;
            s.audio_visualizer = p.audio_visualizer;
            return true;
        }
    }
    return false;
}

bool delete_custom_preset(AppSettings& s, const std::string& name) {
    auto it = std::find_if(s.obs_custom_presets.begin(), s.obs_custom_presets.end(),
                           [&](const ObsCustomPreset& p) { return p.name == name; });
    if (it == s.obs_custom_presets.end()) return false;
    s.obs_custom_presets.erase(it);
    if (s.obs_widget.preset == "user:" + name) {
        s.obs_widget.preset = "custom";
    }
    return true;
}

bool parse_hex_rgba(const std::string& hex, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
    if (hex.empty()) return false;
    size_t off = (hex[0] == '#') ? 1 : 0;
    size_t len = hex.size() - off;
    if (len != 6 && len != 8) return false;

    int vals[8]{0};
    for (size_t i = 0; i < len; ++i) {
        vals[i] = hex_val(hex[off + i]);
        if (vals[i] < 0) return false;
    }
    r = static_cast<uint8_t>((vals[0] << 4) | vals[1]);
    g = static_cast<uint8_t>((vals[2] << 4) | vals[3]);
    b = static_cast<uint8_t>((vals[4] << 4) | vals[5]);
    a = (len == 8) ? static_cast<uint8_t>((vals[6] << 4) | vals[7]) : 255;
    return true;
}

std::string format_hex_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", r, g, b, a);
    return buf;
}

std::string hex_to_css_rgba(const std::string& hex) {
    uint8_t r = 15, g = 15, b = 15, a = 224;
    if (!parse_hex_rgba(hex, r, g, b, a)) {
        return "rgba(15, 15, 15, 0.88)";
    }
    char buf[48];
    std::snprintf(buf, sizeof(buf), "rgba(%u, %u, %u, %.3f)", r, g, b, a / 255.0);
    return buf;
}

std::string format_widget_text(const std::string& pattern,
                               const std::string& song,
                               const std::string& artist,
                               const std::string& album,
                               const std::string& player,
                               const std::string& status) {
    std::string out = fmt::format_now_playing(song, artist, album, pattern);
    out = replace_token(std::move(out), "$player", player.empty() ? "Media" : player);
    out = replace_token(std::move(out), "$status_upper", to_upper_ascii(status.empty() ? "idle" : status));
    out = replace_token(std::move(out), "$status", status.empty() ? "idle" : status);
    return out;
}

std::string generate_studio_css_for_widget(const ObsWidgetSettings& w) {
    std::ostringstream css;

    const std::string bg1 = hex_to_css_rgba(w.bg_color);
    const std::string bg2 = hex_to_css_rgba(w.bg_color_2);
    const std::string brd = hex_to_css_rgba(w.border_color);
    const std::string acc = hex_to_css_rgba(w.accent_color);
    const std::string tcol = hex_to_css_rgba(w.title_color);
    const std::string acol = hex_to_css_rgba(w.artist_color);
    const std::string alcol = hex_to_css_rgba(w.album_color);

    css << "/* Generated by OSMV OBS Studio */\n";
    css << "html, body {\n"
        << "    font-family: '" << w.font_family << "', 'Segoe UI', sans-serif;\n"
        << "}\n\n";

    css << ".obs-stack {\n"
        << "    width: " << std::clamp(w.width_px, 240, 1200) << "px;\n";
    if (w.position == "bottom-left") {
        css << "    bottom: " << w.margin_px << "px; left: " << w.margin_px << "px; top: auto; right: auto; transform: none;\n";
    } else if (w.position == "top-right") {
        css << "    top: " << w.margin_px << "px; right: " << w.margin_px << "px; bottom: auto; left: auto; transform: none;\n";
    } else if (w.position == "top-left") {
        css << "    top: " << w.margin_px << "px; left: " << w.margin_px << "px; bottom: auto; right: auto; transform: none;\n";
    } else if (w.position == "center") {
        css << "    top: 50%; left: 50%; right: auto; bottom: auto; transform: translate(-50%, -50%);\n";
    } else {
        css << "    bottom: " << w.margin_px << "px; right: " << w.margin_px << "px; top: auto; left: auto; transform: none;\n";
    }
    css << "}\n\n";

    css << ".widget-container {\n"
        << "    width: 100%;\n"
        << "    max-width: none;\n"
        << "    padding: " << std::clamp(w.padding_px, 0, 64) << "px;\n"
        << "    border-radius: " << std::clamp(w.border_radius_px, 0, 999) << "px;\n";
    if (w.use_bg_gradient) {
        css << "    background: linear-gradient(135deg, " << bg1 << ", " << bg2 << ");\n";
    } else {
        css << "    background: " << bg1 << ";\n";
    }
    if (w.backdrop_blur_px > 0) {
        css << "    backdrop-filter: blur(" << w.backdrop_blur_px << "px);\n"
            << "    -webkit-backdrop-filter: blur(" << w.backdrop_blur_px << "px);\n";
    }
    if (w.border_width_px > 0) {
        css << "    border: " << w.border_width_px << "px solid " << brd << ";\n";
    } else {
        css << "    border: none;\n";
    }
    if (w.shadow_intensity > 0.01f || w.glow_enabled) {
        css << "    box-shadow: ";
        bool first_shadow = true;
        if (w.shadow_intensity > 0.01f) {
            char sbuf[80];
            std::snprintf(sbuf, sizeof(sbuf), "0 10px 32px rgba(0, 0, 0, %.2f)", w.shadow_intensity);
            css << sbuf;
            first_shadow = false;
        }
        if (w.glow_enabled) {
            if (!first_shadow) css << ", ";
            css << "0 0 24px " << acc;
        }
        css << ";\n";
    } else {
        css << "    box-shadow: none;\n";
    }
    css << "}\n\n";

    css << ".content {\n"
        << "    gap: " << std::clamp(w.gap_px, 0, 64) << "px;\n"
        << "}\n\n";

    css << ".album-art, .album-art-placeholder {\n"
        << "    width: " << std::clamp(w.art_size_px, 24, 240) << "px;\n"
        << "    height: " << std::clamp(w.art_size_px, 24, 240) << "px;\n"
        << "    border-radius: " << std::clamp(w.art_radius_px, 0, 999) << "px;\n"
        << "}\n\n";

    css << ".track-info {\n"
        << "    text-align: " << w.text_align << ";\n"
        << "    align-items: " << (w.text_align == "center" ? "center" : (w.text_align == "right" ? "flex-end" : "flex-start")) << ";\n"
        << "}\n\n";

    css << ".track-badge {\n"
        << "    color: #ffffff;\n"
        << "    background: " << acc << ";\n"
        << "}\n\n";

    css << ".track-title {\n"
        << "    font-size: " << std::clamp(w.title_size_px, 10, 64) << "px;\n"
        << "    font-weight: " << std::clamp(w.title_weight, 100, 900) << ";\n"
        << "    color: " << tcol << ";\n"
        << "    text-shadow: " << (w.text_shadow ? "0 2px 6px rgba(0, 0, 0, 0.55)" : "none") << ";\n"
        << "}\n\n";

    css << ".track-artist {\n"
        << "    font-size: " << std::clamp(w.artist_size_px, 8, 48) << "px;\n"
        << "    color: " << acol << ";\n"
        << "    text-shadow: " << (w.text_shadow ? "0 1px 4px rgba(0, 0, 0, 0.45)" : "none") << ";\n"
        << "}\n\n";

    css << ".track-album {\n"
        << "    font-size: " << std::clamp(w.album_size_px, 8, 36) << "px;\n"
        << "    color: " << alcol << ";\n"
        << "    text-shadow: " << (w.text_shadow ? "0 1px 4px rgba(0, 0, 0, 0.40)" : "none") << ";\n"
        << "}\n\n";

    css << ".visualizer-wrapper, #visualizer {\n"
        << "    height: " << std::clamp(w.viz_height_px, 16, 160) << "px;\n"
        << "}\n";

    return css.str();
}

std::string generate_studio_css(const AppSettings& s) {
    return generate_studio_css_for_widget(s.obs_widget);
}

} // namespace osmv::obs

