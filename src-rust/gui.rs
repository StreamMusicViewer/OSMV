// src-rust/gui.rs
// OsmvApp — premium dark-glassmorphism egui UI with two tabs:
//   1. "Now Playing" — album art, track info, toggles
//   2. "Discord RPC" — full rich presence configuration

use std::sync::{Arc, Mutex};

use eframe::egui::{self, ColorImage, Context, FontData, FontDefinitions, FontFamily, TextureHandle};

use crate::app::{start_background, AppState};
use crate::utils::{load_settings, save_settings};

// Embedded assets
const INTER_FONT: &[u8] = include_bytes!("../assets/Inter-Regular.ttf");
const INTER_BOLD: &[u8] = include_bytes!("../assets/Inter-Bold.ttf");

// Colours
const ACCENT: egui::Color32 = egui::Color32::from_rgb(99, 102, 241);
const ACCENT_DIM: egui::Color32 = egui::Color32::from_rgb(55, 58, 140);
const BG_DARK: egui::Color32 = egui::Color32::from_rgb(12, 12, 18);
const BG_CARD: egui::Color32 = egui::Color32::from_rgba_premultiplied(22, 22, 36, 230);
const BG_INPUT: egui::Color32 = egui::Color32::from_rgb(18, 18, 28);
const TEXT_MAIN: egui::Color32 = egui::Color32::from_rgb(230, 230, 245);
const TEXT_SUB: egui::Color32 = egui::Color32::from_rgb(140, 140, 165);
const TEXT_DIM: egui::Color32 = egui::Color32::from_rgb(70, 70, 100);
const DISCORD_BLURPLE: egui::Color32 = egui::Color32::from_rgb(88, 101, 242);
const DISCORD_BLURPLE_DIM: egui::Color32 = egui::Color32::from_rgb(50, 60, 160);
const GREEN: egui::Color32 = egui::Color32::from_rgb(34, 197, 94);
const YELLOW: egui::Color32 = egui::Color32::from_rgb(234, 179, 8);
const RED: egui::Color32 = egui::Color32::from_rgb(239, 68, 68);

#[derive(PartialEq, Clone, Copy)]
enum Tab { NowPlaying, Discord }

pub struct OsmvApp {
    state: Arc<Mutex<AppState>>,
    thumbnail_handle: Option<TextureHandle>,
    last_thumb_key: String,
    // Cached display values (read each frame from the mutex)
    cached_title: String,
    cached_artist: String,
    cached_album: String,
    cached_status: String,
    cached_dyn_color: bool,
    cached_visualizer: bool,
    // Discord tab — local editable copies (applied on Save)
    dc_enabled: bool,
    dc_use_music: bool,
    dc_client_id: String,
    dc_details: String,
    dc_state: String,
    dc_large_key: String,
    dc_large_text: String,
    dc_small_key: String,
    dc_small_text: String,
    // UI state
    active_tab: Tab,
    pulse_t: f32,
}

impl OsmvApp {
    pub fn new(cc: &eframe::CreationContext<'_>) -> Self {
        setup_fonts(&cc.egui_ctx);
        setup_visuals(&cc.egui_ctx);

        let settings = load_settings();
        let dc = &settings.discord;
        let s = Self {
            state: start_background(settings.clone()),
            thumbnail_handle: None,
            last_thumb_key: String::new(),
            cached_title: "Waiting for music…".into(),
            cached_artist: String::new(),
            cached_album: String::new(),
            cached_status: "closed".into(),
            cached_dyn_color: settings.dynamic_color,
            cached_visualizer: settings.audio_visualizer,
            dc_enabled: dc.enabled,
            dc_use_music: dc.use_music_when_playing,
            dc_client_id: dc.client_id.clone(),
            dc_details: dc.custom_details.clone(),
            dc_state: dc.custom_state.clone(),
            dc_large_key: dc.large_image_key.clone(),
            dc_large_text: dc.large_image_text.clone(),
            dc_small_key: dc.small_image_key.clone(),
            dc_small_text: dc.small_image_text.clone(),
            active_tab: Tab::NowPlaying,
            pulse_t: 0.0,
        };
        s
    }

    fn sync_state(&mut self, ctx: &Context) {
        let locked = self.state.lock().unwrap();
        let song = &locked.song;
        self.cached_title = if song.title.is_empty() { "Waiting for music…".into() } else { song.title.clone() };
        self.cached_artist = song.artist.clone();
        self.cached_album = song.album.clone();
        self.cached_status = song.status.clone();
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

    fn save_media_setting_bool(&self, dyn_color: bool, visualizer: bool) {
        let mut locked = self.state.lock().unwrap();
        locked.settings.dynamic_color = dyn_color;
        locked.settings.audio_visualizer = visualizer;
        save_settings(&locked.settings);
    }

    fn save_discord_settings(&self) {
        let mut locked = self.state.lock().unwrap();
        let dc = &mut locked.settings.discord;
        dc.enabled = self.dc_enabled;
        dc.use_music_when_playing = self.dc_use_music;
        dc.client_id = self.dc_client_id.clone();
        dc.custom_details = self.dc_details.clone();
        dc.custom_state = self.dc_state.clone();
        dc.large_image_key = self.dc_large_key.clone();
        dc.large_image_text = self.dc_large_text.clone();
        dc.small_image_key = self.dc_small_key.clone();
        dc.small_image_text = self.dc_small_text.clone();
        save_settings(&locked.settings);
    }
}

impl eframe::App for OsmvApp {
    fn update(&mut self, ctx: &Context, _frame: &mut eframe::Frame) {
        ctx.request_repaint_after(std::time::Duration::from_millis(1000));
        self.sync_state(ctx);
        self.pulse_t = (self.pulse_t + 0.03) % (2.0 * std::f32::consts::PI);

        egui::CentralPanel::default()
            .frame(egui::Frame::NONE.fill(BG_DARK))
            .show(ctx, |ui| {
                ui.add_space(12.0);
                self.draw_tab_bar(ui);
                ui.add_space(10.0);
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
            ui.add_space(16.0);
            self.tab_button(ui, Tab::NowPlaying, "🎵  Now Playing");
            ui.add_space(6.0);
            self.tab_button(ui, Tab::Discord, "🎮  Discord RPC");
        });
    }

    fn tab_button(&mut self, ui: &mut egui::Ui, tab: Tab, label: &str) {
        let active = self.active_tab == tab;
        let (bg, txt) = if active {
            (ACCENT, egui::Color32::WHITE)
        } else {
            (BG_CARD, TEXT_SUB)
        };

        let btn = egui::Button::new(
            egui::RichText::new(label)
                .font(egui::FontId::new(12.0, egui::FontFamily::Name("Inter-Bold".into())))
                .color(txt),
        )
        .fill(bg)
        .corner_radius(8.0)
        .min_size(egui::vec2(130.0, 30.0));

        if ui.add(btn).clicked() {
            self.active_tab = tab;
        }
    }
}

// ── Now Playing tab ───────────────────────────────────────────────────────────

impl OsmvApp {
    fn draw_now_playing(&mut self, ui: &mut egui::Ui) {
        let avail = ui.available_rect_before_wrap();
        let card = avail.shrink(14.0);

        // Glow + card
        draw_card(ui, card);

        let mut child = ui.new_child(egui::UiBuilder::new().max_rect(card.shrink(16.0)));
        child.horizontal(|ui| {
            self.draw_album_art(ui);
            ui.add_space(16.0);
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
        let sz = egui::vec2(110.0, 110.0);
        let (rect, _) = ui.allocate_exact_size(sz, egui::Sense::hover());
        let p = ui.painter();
        p.rect_filled(rect, 12.0, egui::Color32::from_rgb(20, 20, 32));

        if let Some(h) = &self.thumbnail_handle {
            p.image(h.id(), rect, egui::Rect::from_min_max(egui::pos2(0.0, 0.0), egui::pos2(1.0, 1.0)), egui::Color32::WHITE);
            p.rect_stroke(rect, 12.0, egui::Stroke::new(1.5, egui::Color32::from_rgba_premultiplied(255, 255, 255, 25)), egui::StrokeKind::Middle);
        } else {
            p.rect_stroke(rect, 12.0, egui::Stroke::new(1.5, egui::Color32::from_rgba_premultiplied(99, 102, 241, 50)), egui::StrokeKind::Middle);
            p.text(rect.center(), egui::Align2::CENTER_CENTER, "🎵", egui::FontId::proportional(36.0), TEXT_DIM);
        }
    }

    fn draw_track_info(&self, ui: &mut egui::Ui) {
        ui.add(egui::Label::new(
            egui::RichText::new(&self.cached_title)
                .color(TEXT_MAIN)
                .font(egui::FontId::new(15.0, egui::FontFamily::Name("Inter-Bold".into()))),
        ).wrap());

        if !self.cached_artist.is_empty() && self.cached_artist != "Unknown Artist" {
            ui.add(egui::Label::new(
                egui::RichText::new(&self.cached_artist).color(TEXT_SUB).font(egui::FontId::proportional(12.5)),
            ).wrap());
        }
        if !self.cached_album.is_empty() {
            ui.add(egui::Label::new(
                egui::RichText::new(format!("💿 {}", self.cached_album)).color(TEXT_DIM).font(egui::FontId::proportional(11.0)),
            ).wrap());
        }
    }

    fn draw_status_badge(&self, ui: &mut egui::Ui) {
        let (color, dot_color, label) = match self.cached_status.as_str() {
            "playing" => (GREEN, egui::Color32::from_rgb(74, 222, 128), "▶  Playing"),
            "paused"  => (YELLOW, egui::Color32::from_rgb(253, 224, 71), "⏸  Paused"),
            "stopped" => (RED, egui::Color32::from_rgb(252, 165, 165),  "⏹  Stopped"),
            _         => (TEXT_DIM, TEXT_DIM, "—  Idle"),
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
        let mut dc = self.cached_dyn_color;
        let mut viz = self.cached_visualizer;
        let dc_changed = ui.checkbox(&mut dc, egui::RichText::new("🎨  Match cover color").color(TEXT_SUB).font(egui::FontId::proportional(11.5))).changed();
        let viz_changed = ui.checkbox(&mut viz, egui::RichText::new("🎚  Audio visualizer (beta)").color(TEXT_SUB).font(egui::FontId::proportional(11.5))).changed();
        if dc_changed || viz_changed {
            self.cached_dyn_color = dc;
            self.cached_visualizer = viz;
            self.save_media_setting_bool(dc, viz);
        }
    }
}

// ── Discord RPC tab ───────────────────────────────────────────────────────────

impl OsmvApp {
    fn draw_discord(&mut self, ui: &mut egui::Ui) {
        let avail = ui.available_rect_before_wrap();
        let card = avail.shrink(14.0);
        draw_discord_card(ui, card);

        egui::ScrollArea::vertical()
            .id_salt("discord_scroll")
            .show(ui, |ui| {
                ui.add_space(16.0);
                self.draw_discord_inner(ui);
            });
    }

    fn draw_discord_inner(&mut self, ui: &mut egui::Ui) {
        let w = ui.available_width();

        // ── Enable toggle ─────────────────────────────────────────────────────
        ui.horizontal(|ui| {
            ui.add_space(20.0);
            let label = if self.dc_enabled {
                egui::RichText::new("● Discord Rich Presence  ON").color(egui::Color32::from_rgb(88, 101, 242)).font(egui::FontId::new(13.0, egui::FontFamily::Name("Inter-Bold".into())))
            } else {
                egui::RichText::new("○ Discord Rich Presence  OFF").color(TEXT_SUB).font(egui::FontId::new(13.0, egui::FontFamily::Name("Inter-Bold".into())))
            };
            ui.toggle_value(&mut self.dc_enabled, label);
        });

        ui.add_space(12.0);
        ui.separator();
        ui.add_space(10.0);

        // ── Client ID ────────────────────────────────────────────────────────
        section_label(ui, "Application Client ID");
        small_hint(ui, "Create an app at discord.com/developers → copy the Application ID");
        ui.add_space(4.0);
        text_field(ui, &mut self.dc_client_id, "e.g. 1234567890123456789", w - 40.0);

        ui.add_space(12.0);

        // ── Music integration ────────────────────────────────────────────────
        section_label(ui, "Music Integration");
        ui.horizontal(|ui| {
            ui.add_space(20.0);
            ui.checkbox(
                &mut self.dc_use_music,
                egui::RichText::new("Show current song as activity when music is playing")
                    .color(TEXT_SUB).font(egui::FontId::proportional(11.5)),
            );
        });
        small_hint(ui, "When no music plays, the custom activity below is used instead.");

        ui.add_space(12.0);
        ui.separator();
        ui.add_space(10.0);

        // ── Custom activity ──────────────────────────────────────────────────
        section_label(ui, "Default / Fallback Activity");

        ui.horizontal(|ui| {
            ui.add_space(20.0);
            ui.vertical(|ui| {
                ui.label(egui::RichText::new("Details  (line 1)").color(TEXT_SUB).font(egui::FontId::proportional(11.0)));
                text_field(ui, &mut self.dc_details, "e.g. Streaming on OBS", (w - 40.0) / 2.0 - 8.0);
            });
            ui.add_space(8.0);
            ui.vertical(|ui| {
                ui.label(egui::RichText::new("State  (line 2)").color(TEXT_SUB).font(egui::FontId::proportional(11.0)));
                text_field(ui, &mut self.dc_state, "e.g. Playing games", (w - 40.0) / 2.0 - 8.0);
            });
        });

        ui.add_space(12.0);
        ui.separator();
        ui.add_space(10.0);

        // ── Images ───────────────────────────────────────────────────────────
        section_label(ui, "Images  (Art Asset keys from your Discord app)");

        ui.horizontal(|ui| {
            ui.add_space(20.0);
            ui.vertical(|ui| {
                ui.label(egui::RichText::new("Large image key").color(TEXT_SUB).font(egui::FontId::proportional(11.0)));
                text_field(ui, &mut self.dc_large_key, "e.g. osmv_logo", (w - 40.0) / 2.0 - 8.0);
                ui.add_space(4.0);
                ui.label(egui::RichText::new("Large image tooltip").color(TEXT_SUB).font(egui::FontId::proportional(11.0)));
                text_field(ui, &mut self.dc_large_text, "Hover text", (w - 40.0) / 2.0 - 8.0);
            });
            ui.add_space(8.0);
            ui.vertical(|ui| {
                ui.label(egui::RichText::new("Small image key").color(TEXT_SUB).font(egui::FontId::proportional(11.0)));
                text_field(ui, &mut self.dc_small_key, "e.g. play_icon", (w - 40.0) / 2.0 - 8.0);
                ui.add_space(4.0);
                ui.label(egui::RichText::new("Small image tooltip").color(TEXT_SUB).font(egui::FontId::proportional(11.0)));
                text_field(ui, &mut self.dc_small_text, "Hover text", (w - 40.0) / 2.0 - 8.0);
            });
        });

        ui.add_space(16.0);

        // ── Save button ───────────────────────────────────────────────────────
        ui.horizontal(|ui| {
            ui.add_space(20.0);
            let save_btn = egui::Button::new(
                egui::RichText::new("  💾  Save Settings  ")
                    .font(egui::FontId::new(12.5, egui::FontFamily::Name("Inter-Bold".into())))
                    .color(egui::Color32::WHITE),
            )
            .fill(DISCORD_BLURPLE)
            .corner_radius(8.0)
            .min_size(egui::vec2(160.0, 32.0));

            if ui.add(save_btn).clicked() {
                self.save_discord_settings();
            }
        });

        ui.add_space(20.0);
    }
}

// ── Shared drawing helpers ────────────────────────────────────────────────────

fn draw_card(ui: &mut egui::Ui, rect: egui::Rect) {
    let p = ui.painter();
    for i in 0..6u8 {
        let e = (i as f32) * 2.0;
        let a = 20u8.saturating_sub(i * 3);
        p.rect_filled(rect.expand(e), 14.0 + e * 0.5, egui::Color32::from_rgba_premultiplied(99, 102, 241, a));
    }
    p.rect_filled(rect, 14.0, BG_CARD);
    p.rect_stroke(rect, 14.0, egui::Stroke::new(1.0, egui::Color32::from_rgba_premultiplied(99, 102, 241, 70)), egui::StrokeKind::Middle);
}

fn draw_discord_card(ui: &mut egui::Ui, rect: egui::Rect) {
    let p = ui.painter();
    for i in 0..6u8 {
        let e = (i as f32) * 2.0;
        let a = 18u8.saturating_sub(i * 3);
        p.rect_filled(rect.expand(e), 14.0 + e * 0.5, egui::Color32::from_rgba_premultiplied(88, 101, 242, a));
    }
    p.rect_filled(rect, 14.0, BG_CARD);
    p.rect_stroke(rect, 14.0, egui::Stroke::new(1.0, egui::Color32::from_rgba_premultiplied(88, 101, 242, 70)), egui::StrokeKind::Middle);
}

fn section_label(ui: &mut egui::Ui, text: &str) {
    ui.horizontal(|ui| {
        ui.add_space(20.0);
        ui.label(egui::RichText::new(text)
            .font(egui::FontId::new(12.0, egui::FontFamily::Name("Inter-Bold".into())))
            .color(TEXT_MAIN));
    });
    ui.add_space(4.0);
}

fn small_hint(ui: &mut egui::Ui, text: &str) {
    ui.horizontal(|ui| {
        ui.add_space(20.0);
        ui.label(egui::RichText::new(text).font(egui::FontId::proportional(10.5)).color(TEXT_DIM));
    });
}

fn text_field(ui: &mut egui::Ui, value: &mut String, hint: &str, width: f32) {
    let te = egui::TextEdit::singleline(value)
        .hint_text(egui::RichText::new(hint).color(TEXT_DIM))
        .font(egui::FontId::proportional(12.0))
        .text_color(TEXT_MAIN)
        .background_color(BG_INPUT)
        .desired_width(width)
        .margin(egui::Margin::symmetric(8, 5));
    ui.add(te);
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
    v.window_fill = BG_DARK;
    v.panel_fill = BG_DARK;
    v.faint_bg_color = BG_CARD;
    v.override_text_color = Some(TEXT_MAIN);
    v.widgets.inactive.bg_fill = egui::Color32::from_rgb(28, 28, 48);
    v.widgets.inactive.fg_stroke = egui::Stroke::new(1.0, TEXT_DIM);
    v.widgets.active.bg_fill = ACCENT;
    v.widgets.active.fg_stroke = egui::Stroke::new(1.5, egui::Color32::WHITE);
    v.widgets.hovered.bg_fill = ACCENT_DIM;
    v.widgets.hovered.fg_stroke = egui::Stroke::new(1.5, TEXT_MAIN);
    v.selection.bg_fill = DISCORD_BLURPLE_DIM;
    // Text cursor
    v.text_cursor.stroke = egui::Stroke::new(2.0, ACCENT);
    ctx.set_visuals(v);
}
