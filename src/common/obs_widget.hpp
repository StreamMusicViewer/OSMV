#pragma once

#include "types.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace osmv::obs {

struct PresetInfo {
    const char* id;
    const char* name_en;
    const char* name_fr;
    const char* desc_en;
    const char* desc_fr;
};

const std::vector<PresetInfo>& preset_catalog();
bool apply_preset(ObsWidgetSettings& w, const std::string& preset_id);
void reset_widget_defaults(AppSettings& s);

bool upsert_custom_preset(AppSettings& s, const std::string& name);
bool apply_custom_preset(AppSettings& s, const std::string& name);
bool delete_custom_preset(AppSettings& s, const std::string& name);

bool parse_hex_rgba(const std::string& hex, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a);
std::string format_hex_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
std::string hex_to_css_rgba(const std::string& hex);

std::string format_widget_text(const std::string& pattern,
                               const std::string& song,
                               const std::string& artist,
                               const std::string& album,
                               const std::string& player,
                               const std::string& status);

// Generates the complete CSS stylesheet corresponding to widget settings.
std::string generate_studio_css_for_widget(const ObsWidgetSettings& w);
std::string generate_studio_css(const AppSettings& s);

} // namespace osmv::obs

