// src-rust/gui.rs
// OsmvApp — premium dark UI with two tabs: Now Playing | Discord RPC

use std::sync::{Arc, Mutex};

use eframe::egui::{self, ColorImage, Context, FontData, FontDefinitions, FontFamily, TextureHandle};

use crate::app::{start_background, AppState};
use crate::utils::{load_settings, save_settings};

const INTER_FONT: &[u8] = include_bytes!("../assets/Inter-Regular.ttf");
const INTER_BOLD: &[u8] = include_bytes!("../assets/Inter-Bold.ttf");

// Colour palette
const ACCENT:            egui::Color32 = egui::Color32::from_rgb(99, 102, 241);
const ACCENT_DIM:        egui::Color32 = egui::Color32::from_rgb(55, 58, 140);
const BG_DARK:           egui::Color32 = egui::Color32::from_rgb(12, 12, 18);
const BG_CARD:           egui::Color32 = egui::Color32::from_rgba_premultiplied(22, 22, 36, 230);
const BG_INPUT:          egui::Color32 = egui::Color32::from_rgb(18, 18, 30);
const TEXT_MAIN:         egui::Color32 = egui::Color32::from_rgb(230, 230, 245);
const TEXT_SUB:          egui::Color32 = egui::Color32::from_rgb(140, 140, 165);
const TEXT_DIM:          egui::Color32 = egui::Color32::from_rgb(70, 70, 100);
const BLURPLE:           egui::Color32 = egui::Color32::from_rgb(88, 101, 242);
const BLURPLE_DIM:       egui::Color32 = egui::Color32::from_rgb(50, 60, 160);
const COL_GREEN:         egui::Color32 = egui::Color32::from_rgb(34, 197, 94);
const COL_YELLOW:        egui::Color32 = egui::Color32::from_rgb(234, 179, 8);
const COL_RED:           egui::Color32 = egui::Color32::from_rgb(239, 68, 68);

#[derive(PartialEq, Clone, Copy)]
enum Tab { NowPlaying, Discord }

pub struct OsmvApp {
    state: Arc<Mutex<AppState>>,
    thumbnail_handle: Option<TextureHandle>,
    last_thumb_key: String,

    // Cached (read each frame)
    cached_title:      String,
    cached_artist:     String,
    cached_album:      String,
    cached_status:     String,
    cached_dyn_color:  bool,
    cached_visualizer: bool,

    // Discord form (local editable copy; applied on Save)
    dc_enabled:      bool,
    dc_use_music:    bool,
    dc_client_id:    String,
    dc_details:      String,
    dc_state:        String,
    dc_large_key:    String,
    dc_large_text:   String,
    dc_key_playing:  String,
    dc_key_paused:   String,
    dc_key_stopped:  String,
    dc_small_key:    String,
    dc_small_text:   String,

    active_tab: Tab,
    pulse_t: f32,
}

impl OsmvApp {
    pub fn new(cc: &eframe::CreationContext<'_>) -> Self {
        setup_fonts(&cc.egui_ctx);
        setup_visuals(&cc.egui_ctx);

        let settings = load_settings();
        let dc = &settings.discord;

        Self {
            state: start_background(settings.clone()),
            thumbnail_handle: None,
            last_thumb_key: String::new(),
            cached_title: "Waiting for music...".into(),
            cached_artist: String::new(),
            cached_album: String::new(),
            cached_status: "closed".into(),
            cached_dyn_color: settings.dynamic_color,
            cached_visualizer: settings.audio_visualizer,
            dc_enabled:     dc.enabled,
            dc_use_music:   dc.use_music_when_playing,
            dc_client_id:   dc.client_id.clone(),
            dc_details:     dc.custom_details.clone(),
            dc_state:       dc.custom_state.clone(),
            dc_large_key:   dc.large_image_key.clone(),
            dc_large_text:  dc.large_image_text.clone(),
            dc_key_playing: dc.status_key_playing.clone(),
            dc_key_paused:  dc.status_key_paused.clone(),
            dc_key_stopped: dc.status_key_stopped.clone(),
            dc_small_key:   dc.small_image_key.clone(),
            dc_small_text:  dc.small_image_text.clone(),
            active_tab: Tab::NowPlaying,
            pulse_t: 0.0,
        }
    }

    fn sync_state(&mut self, ctx: &Context) {
        let locked = self.state.lock().unwrap();
        let song = &locked.song;
        self.cached_title     = if song.title.is_empty() { "Waiting for music...".into() } else { song.title.clone() };
        self.cached_artist    = song.artist.clone();
        self.cached_album     = song.album.clone();
        self.cached_status    = song.status.clone();
        self.cached_dyn_color = locked.settings.dynamic_color;
        self.cached_visualizer = locked.settings.audio_visualizer;

        let key = locked.last_song_key.clone();
        if key != self.last_thumb_key {
            self.last_thumb_key = key;
            self.thumbnail_handle = None;
            if let Some(bytes) = &locked.thumbnail_bytes {
                if let Ok(img) = image::load_from_memory(bytes) {
                    let rgba = img.to_rgba8();
                    let (w, h) = rgba.dimensions();
                    let ci = ColorImage::from_rgba_unmultiplied([w as usize, h as usize], rgba.as_raw());
                    self.thumbnail_handle = Some(ctx.load_texture("album_art", ci, egui::TextureOptions::LINEAR));
                }
            }
        }
    }

    fn save_media_settings(&self, dyn_color: bool, visualizer: bool) {
        let mut locked = self.state.lock().unwrap();
        locked.settings.dynamic_color  = dyn_color;
        locked.settings.audio_visualizer = visualizer;
        save_settings(&locked.settings);
    }

    fn save_discord_settings(&self) {
        let mut locked = self.state.lock().unwrap();
        let dc = &mut locked.settings.discord;
        dc.enabled                = self.dc_enabled;
        dc.use_music_when_playing = self.dc_use_music;
        dc.client_id              = self.dc_client_id.clone();
        dc.custom_details         = self.dc_details.clone();
        dc.custom_state           = self.dc_state.clone();
        dc.large_image_key        = self.dc_large_key.clone();
        dc.large_image_text       = self.dc_large_text.clone();
        dc.status_key_playing     = self.dc_key_playing.clone();
        dc.status_key_paused      = self.dc_key_paused.clone();
        dc.status_key_stopped     = self.dc_key_stopped.clone();
        dc.small_image_key        = self.dc_small_key.clone();
        dc.small_image_text       = self.dc_small_text.clone();
        save_settings(&locked.settings);
    }
}

// ── eframe::App ───────────────────────────────────────────────────────────────

impl eframe::App for OsmvApp {
    fn update(&mut self, ctx: &Context, _frame: &mut eframe::Frame) {
        ctx.request_repaint_after(std::time::Duration::from_millis(1000));
        self.sync_state(ctx);
        self.pulse_t = (self.pulse_t + 0.03) % (2.0 * std::f32::consts::PI);

        egui::CentralPanel::default()
            .frame(egui::Frame::NONE.fill(BG_DARK))
            .show(ctx, |ui| {
                ui.add_space(10.0);
                self.draw_tab_bar(ui);
                ui.add_space(8.0);
                match self.active_tab {
                    Tab::NowPlaying => self.draw_now_playing(ui),
                    Tab::Discord    => self.draw_discord(ui),
                }
            });
    }
}

// ── Tab bar ───────────────────────────────────────────────────────────────────

impl OsmvApp {
    fn draw_tab_bar(&mut self, ui: &mut egui::Ui) {
        ui.horizontal(|ui| {
            ui.add_space(14.0);
            for (tab, label) in [(Tab::NowPlaying, "Now Playing"), (Tab::Discord, "Discord RPC")] {
                let active = self.active_tab == tab;
                let btn = egui::Button::new(
                    egui::RichText::new(label)
                        .font(egui::FontId::new(12.0, egui::FontFamily::Name("Inter-Bold".into())))
                        .color(if active { egui::Color32::WHITE } else { TEXT_SUB }),
                )
                .fill(if active { ACCENT } else { BG_CARD })
                .corner_radius(8.0)
                .min_size(egui::vec2(120.0, 28.0));
                if ui.add(btn).clicked() { self.active_tab = tab; }
                ui.add_space(4.0);
            }
        });
    }
}

// ── Now Playing tab ───────────────────────────────────────────────────────────

impl OsmvApp {
    fn draw_now_playing(&mut self, ui: &mut egui::Ui) {
        let card = ui.available_rect_before_wrap().shrink(14.0);
        glow_card(ui, card, ACCENT);

        let mut child = ui.new_child(egui::UiBuilder::new().max_rect(card.shrink(14.0)));
        child.horizontal(|ui| {
            self.draw_album_art(ui);
            ui.add_space(14.0);
            ui.vertical(|ui| {
                self.draw_track_info(ui);
                ui.add_space(8.0);
                self.draw_status_badge(ui);
                ui.add_space(10.0);
                self.draw_media_toggles(ui);
            });
        });
    }

    fn draw_album_art(&self, ui: &mut egui::Ui) {
        let (rect, _) = ui.allocate_exact_size(egui::vec2(108.0, 108.0), egui::Sense::hover());
        let p = ui.painter();
        p.rect_filled(rect, 10.0, egui::Color32::from_rgb(20, 20, 32));
        if let Some(h) = &self.thumbnail_handle {
            p.image(h.id(), rect, egui::Rect::from_min_max(egui::pos2(0.0, 0.0), egui::pos2(1.0, 1.0)), egui::Color32::WHITE);
            p.rect_stroke(rect, 10.0, egui::Stroke::new(1.0, egui::Color32::from_rgba_premultiplied(255, 255, 255, 20)), egui::StrokeKind::Middle);
        } else {
            p.rect_stroke(rect, 10.0, egui::Stroke::new(1.0, egui::Color32::from_rgba_premultiplied(99, 102, 241, 40)), egui::StrokeKind::Middle);
            p.text(rect.center(), egui::Align2::CENTER_CENTER, "?", egui::FontId::proportional(36.0), TEXT_DIM);
        }
    }

    fn draw_track_info(&self, ui: &mut egui::Ui) {
        ui.add(egui::Label::new(
            egui::RichText::new(&self.cached_title)
                .color(TEXT_MAIN)
                .font(egui::FontId::new(14.5, egui::FontFamily::Name("Inter-Bold".into()))),
        ).wrap());
        if !self.cached_artist.is_empty() && self.cached_artist != "Unknown Artist" {
            ui.add(egui::Label::new(
                egui::RichText::new(&self.cached_artist).color(TEXT_SUB).font(egui::FontId::proportional(12.0)),
            ).wrap());
        }
        if !self.cached_album.is_empty() {
            ui.add(egui::Label::new(
                egui::RichText::new(format!("Album: {}", self.cached_album)).color(TEXT_DIM).font(egui::FontId::proportional(11.0)),
            ).wrap());
        }
    }

    fn draw_status_badge(&self, ui: &mut egui::Ui) {
        let (color, dot_color, label) = match self.cached_status.as_str() {
            "playing" => (COL_GREEN,  egui::Color32::from_rgb(74, 222, 128), "Playing"),
            "paused"  => (COL_YELLOW, egui::Color32::from_rgb(253, 224, 71),  "Paused"),
            "stopped" => (COL_RED,    egui::Color32::from_rgb(252, 165, 165), "Stopped"),
            _         => (TEXT_DIM,   TEXT_DIM,                               "Idle"),
        };
        ui.horizontal(|ui| {
            let sz = if self.cached_status == "playing" {
                let p = (self.pulse_t.sin() * 0.5 + 0.5) * 2.0 + 5.0;
                egui::vec2(p, p)
            } else { egui::vec2(7.0, 7.0) };
            let (dr, _) = ui.allocate_exact_size(sz, egui::Sense::hover());
            ui.painter().circle_filled(dr.center(), sz.x / 2.0, dot_color);
            ui.label(egui::RichText::new(label).color(color).font(egui::FontId::proportional(11.5)));
        });
    }

    fn draw_media_toggles(&mut self, ui: &mut egui::Ui) {
        let mut dc  = self.cached_dyn_color;
        let mut viz = self.cached_visualizer;
        let dc_ch  = ui.checkbox(&mut dc,  egui::RichText::new("Match cover color").color(TEXT_SUB).font(egui::FontId::proportional(11.5))).changed();
        let viz_ch = ui.checkbox(&mut viz, egui::RichText::new("Audio visualizer (beta)").color(TEXT_SUB).font(egui::FontId::proportional(11.5))).changed();
        if dc_ch || viz_ch {
            self.cached_dyn_color  = dc;
            self.cached_visualizer = viz;
            self.save_media_settings(dc, viz);
        }
    }
}

// ── Discord RPC tab ───────────────────────────────────────────────────────────

impl OsmvApp {
    fn draw_discord(&mut self, ui: &mut egui::Ui) {
        let card = ui.available_rect_before_wrap().shrink(14.0);
        glow_card(ui, card, BLURPLE);

        // Scroll area that fits within the card
        let inner = card.shrink(10.0);
        let mut child = ui.new_child(egui::UiBuilder::new().max_rect(inner));
        egui::ScrollArea::vertical()
            .id_salt("discord_scroll")
            .auto_shrink([false, false])
            .show(&mut child, |ui| {
                ui.add_space(10.0);
                self.draw_discord_form(ui);
            });
    }

    fn draw_discord_form(&mut self, ui: &mut egui::Ui) {
        let pad = 16.0;
        let w = ui.available_width() - pad * 2.0;
        let half = (w - 8.0) / 2.0;
        let third = (w - 16.0) / 3.0;

        // ── Enable toggle ─────────────────────────────────────────────────────
        ui.add_space(4.0);
        ui.horizontal(|ui| {
            ui.add_space(pad);
            let (lbl, col) = if self.dc_enabled {
                ("Discord Rich Presence  ON", BLURPLE)
            } else {
                ("Discord Rich Presence  OFF", TEXT_SUB)
            };
            ui.toggle_value(
                &mut self.dc_enabled,
                egui::RichText::new(lbl)
                    .font(egui::FontId::new(12.5, egui::FontFamily::Name("Inter-Bold".into())))
                    .color(col),
            );
        });

        ui.add_space(10.0);
        ui.separator();
        ui.add_space(8.0);

        // ── Client ID ────────────────────────────────────────────────────────
        field_section(ui, pad, "Application Client ID",
            "Create an app at discord.com/developers and copy the Application ID");
        ui.add_space(4.0);
        ui.horizontal(|ui| {
            ui.add_space(pad);
            input_field(ui, &mut self.dc_client_id, "1234567890123456789", w);
        });

        ui.add_space(10.0);
        ui.separator();
        ui.add_space(8.0);

        // ── Music integration ─────────────────────────────────────────────────
        field_section(ui, pad, "Music Integration", "");
        ui.horizontal(|ui| {
            ui.add_space(pad);
            ui.checkbox(
                &mut self.dc_use_music,
                egui::RichText::new("Show current song when music is playing")
                    .color(TEXT_SUB).font(egui::FontId::proportional(11.5)),
            );
        });
        ui.horizontal(|ui| {
            ui.add_space(pad);
            ui.label(egui::RichText::new("When no music plays, the custom activity below is shown.")
                .color(TEXT_DIM).font(egui::FontId::proportional(10.5)));
        });

        ui.add_space(10.0);
        ui.separator();
        ui.add_space(8.0);

        // ── Fallback activity ─────────────────────────────────────────────────
        field_section(ui, pad, "Default / Fallback Activity", "Shown when no music is playing");
        ui.add_space(4.0);
        ui.horizontal(|ui| {
            ui.add_space(pad);
            ui.vertical(|ui| {
                field_label(ui, "Details  (line 1)");
                input_field(ui, &mut self.dc_details, "Streaming on OBS", half);
            });
            ui.add_space(8.0);
            ui.vertical(|ui| {
                field_label(ui, "State  (line 2)");
                input_field(ui, &mut self.dc_state, "Playing games", half);
            });
        });

        ui.add_space(10.0);
        ui.separator();
        ui.add_space(8.0);

        // ── Album cover ───────────────────────────────────────────────────────
        field_section(ui, pad, "Album Cover  (large image)", "");
        ui.horizontal(|ui| {
            ui.add_space(pad);
            ui.label(egui::RichText::new("When music plays, the cover is fetched automatically from iTunes.")
                .color(TEXT_DIM).font(egui::FontId::proportional(10.5)));
        });
        ui.horizontal(|ui| {
            ui.add_space(pad);
            ui.label(egui::RichText::new("Fallback key used when no cover is found (Art Asset in your Discord app):")
                .color(TEXT_DIM).font(egui::FontId::proportional(10.5)));
        });
        ui.add_space(4.0);
        ui.horizontal(|ui| {
            ui.add_space(pad);
            ui.vertical(|ui| {
                field_label(ui, "Fallback large image key");
                input_field(ui, &mut self.dc_large_key, "osmv_logo", half);
            });
            ui.add_space(8.0);
            ui.vertical(|ui| {
                field_label(ui, "Large image tooltip");
                input_field(ui, &mut self.dc_large_text, "Hover text", half);
            });
        });

        ui.add_space(10.0);
        ui.separator();
        ui.add_space(8.0);

        // ── Status icons ──────────────────────────────────────────────────────
        field_section(ui, pad, "Status Icons  (small image, bottom-right)", "");
        ui.horizontal(|ui| {
            ui.add_space(pad);
            ui.label(egui::RichText::new("Upload icons to your Discord app Art Assets with these exact key names.")
                .color(TEXT_DIM).font(egui::FontId::proportional(10.5)));
        });
        ui.add_space(4.0);
        ui.horizontal(|ui| {
            ui.add_space(pad);
            ui.vertical(|ui| {
                field_label_colored(ui, "Playing key", COL_GREEN);
                input_field(ui, &mut self.dc_key_playing, "playing", third);
            });
            ui.add_space(8.0);
            ui.vertical(|ui| {
                field_label_colored(ui, "Paused key", COL_YELLOW);
                input_field(ui, &mut self.dc_key_paused, "paused", third);
            });
            ui.add_space(8.0);
            ui.vertical(|ui| {
                field_label_colored(ui, "Stopped key", COL_RED);
                input_field(ui, &mut self.dc_key_stopped, "stopped", third);
            });
        });

        ui.add_space(10.0);
        ui.separator();
        ui.add_space(8.0);

        // ── Idle small image ──────────────────────────────────────────────────
        field_section(ui, pad, "Custom / Idle Small Image",
            "Shown when no music is playing");
        ui.add_space(4.0);
        ui.horizontal(|ui| {
            ui.add_space(pad);
            ui.vertical(|ui| {
                field_label(ui, "Small image key");
                input_field(ui, &mut self.dc_small_key, "idle_icon", half);
            });
            ui.add_space(8.0);
            ui.vertical(|ui| {
                field_label(ui, "Small image tooltip");
                input_field(ui, &mut self.dc_small_text, "Hover text", half);
            });
        });

        ui.add_space(14.0);

        // ── Save ──────────────────────────────────────────────────────────────
        ui.horizontal(|ui| {
            ui.add_space(pad);
            let btn = egui::Button::new(
                egui::RichText::new("Save Settings")
                    .font(egui::FontId::new(12.0, egui::FontFamily::Name("Inter-Bold".into())))
                    .color(egui::Color32::WHITE),
            )
            .fill(BLURPLE)
            .corner_radius(8.0)
            .min_size(egui::vec2(140.0, 30.0));
            if ui.add(btn).clicked() {
                self.save_discord_settings();
            }
        });

        ui.add_space(16.0);
    }
}

// ── Drawing helpers ───────────────────────────────────────────────────────────

fn glow_card(ui: &mut egui::Ui, rect: egui::Rect, color: egui::Color32) {
    let p = ui.painter();
    for i in 0..6u8 {
        let e = (i as f32) * 2.0;
        let a = 15u8.saturating_sub(i * 2);
        let [r, g, b, _] = color.to_array();
        p.rect_filled(rect.expand(e), 14.0 + e * 0.4, egui::Color32::from_rgba_premultiplied(r, g, b, a));
    }
    p.rect_filled(rect, 14.0, BG_CARD);
    let [r, g, b, _] = color.to_array();
    p.rect_stroke(rect, 14.0,
        egui::Stroke::new(1.0, egui::Color32::from_rgba_premultiplied(r, g, b, 60)),
        egui::StrokeKind::Middle);
}

fn field_section(ui: &mut egui::Ui, pad: f32, title: &str, hint: &str) {
    ui.horizontal(|ui| {
        ui.add_space(pad);
        ui.label(egui::RichText::new(title)
            .font(egui::FontId::new(11.5, egui::FontFamily::Name("Inter-Bold".into())))
            .color(TEXT_MAIN));
    });
    if !hint.is_empty() {
        ui.horizontal(|ui| {
            ui.add_space(pad);
            ui.label(egui::RichText::new(hint).font(egui::FontId::proportional(10.5)).color(TEXT_DIM));
        });
    }
}

fn field_label(ui: &mut egui::Ui, text: &str) {
    ui.label(egui::RichText::new(text).color(TEXT_SUB).font(egui::FontId::proportional(10.5)));
}

fn field_label_colored(ui: &mut egui::Ui, text: &str, color: egui::Color32) {
    ui.label(egui::RichText::new(text).color(color).font(egui::FontId::proportional(10.5)));
}

fn input_field(ui: &mut egui::Ui, value: &mut String, hint: &str, width: f32) {
    ui.add(
        egui::TextEdit::singleline(value)
            .hint_text(egui::RichText::new(hint).color(TEXT_DIM))
            .font(egui::FontId::proportional(11.5))
            .text_color(TEXT_MAIN)
            .background_color(BG_INPUT)
            .desired_width(width.max(60.0))
            .margin(egui::Margin::symmetric(8, 4)),
    );
}

// ── Font + visual setup ───────────────────────────────────────────────────────

fn setup_fonts(ctx: &Context) {
    let mut fonts = FontDefinitions::default();
    fonts.font_data.insert("Inter".into(), FontData::from_static(INTER_FONT).into());
    fonts.font_data.insert("Inter-Bold".into(), FontData::from_static(INTER_BOLD).into());
    fonts.families.entry(FontFamily::Proportional).or_default().insert(0, "Inter".into());
    fonts.families.insert(FontFamily::Name("Inter-Bold".into()), vec!["Inter-Bold".into()]);
    ctx.set_fonts(fonts);
}

fn setup_visuals(ctx: &Context) {
    let mut v = egui::Visuals::dark();
    v.window_fill              = BG_DARK;
    v.panel_fill               = BG_DARK;
    v.faint_bg_color           = BG_CARD;
    v.override_text_color      = Some(TEXT_MAIN);
    v.widgets.inactive.bg_fill = egui::Color32::from_rgb(28, 28, 48);
    v.widgets.inactive.fg_stroke = egui::Stroke::new(1.0, TEXT_DIM);
    v.widgets.active.bg_fill   = ACCENT;
    v.widgets.active.fg_stroke = egui::Stroke::new(1.5, egui::Color32::WHITE);
    v.widgets.hovered.bg_fill  = ACCENT_DIM;
    v.widgets.hovered.fg_stroke = egui::Stroke::new(1.5, TEXT_MAIN);
    v.selection.bg_fill        = BLURPLE_DIM;
    v.text_cursor.stroke       = egui::Stroke::new(2.0, ACCENT);
    ctx.set_visuals(v);
}
