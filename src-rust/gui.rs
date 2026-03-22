// src-rust/gui.rs
// OsmvApp — the egui application. Premium dark-glassmorphism aesthetic.

use std::sync::{Arc, Mutex};

use eframe::egui::{self, ColorImage, Context, FontData, FontDefinitions, FontFamily, TextureHandle};

use crate::app::{start_background, AppState};
use crate::utils::{load_settings, save_settings};

const INTER_FONT: &[u8] = include_bytes!("../assets/Inter-Regular.ttf");
const INTER_BOLD: &[u8] = include_bytes!("../assets/Inter-Bold.ttf");

const ACCENT: egui::Color32 = egui::Color32::from_rgb(99, 102, 241);
const ACCENT_DIM: egui::Color32 = egui::Color32::from_rgb(55, 58, 140);
const BG_DARK: egui::Color32 = egui::Color32::from_rgb(12, 12, 18);
const BG_PANEL: egui::Color32 = egui::Color32::from_rgba_premultiplied(26, 26, 38, 220);
const TEXT_MAIN: egui::Color32 = egui::Color32::from_rgb(230, 230, 245);
const TEXT_SUB: egui::Color32 = egui::Color32::from_rgb(140, 140, 165);
const TEXT_STATUS: egui::Color32 = egui::Color32::from_rgb(80, 80, 110);

pub struct OsmvApp {
    state: Arc<Mutex<AppState>>,
    thumbnail_handle: Option<TextureHandle>,
    last_thumb_key: String,
    cached_title: String,
    cached_artist: String,
    cached_album: String,
    cached_status: String,
    cached_dyn_color: bool,
    cached_visualizer: bool,
    pulse_t: f32,
}

impl OsmvApp {
    pub fn new(cc: &eframe::CreationContext<'_>) -> Self {
        setup_fonts(&cc.egui_ctx);
        setup_visuals(&cc.egui_ctx);

        let settings = load_settings();
        let state = start_background(settings.clone());

        Self {
            state,
            thumbnail_handle: None,
            last_thumb_key: String::new(),
            cached_title: "Waiting for music…".into(),
            cached_artist: String::new(),
            cached_album: String::new(),
            cached_status: "closed".into(),
            cached_dyn_color: settings.dynamic_color,
            cached_visualizer: settings.audio_visualizer,
            pulse_t: 0.0,
        }
    }

    fn sync_state(&mut self, ctx: &Context) {
        let locked = self.state.lock().unwrap();
        let song = &locked.song;

        self.cached_title = if song.title.is_empty() {
            "Waiting for music…".into()
        } else {
            song.title.clone()
        };
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
                    let color_image = ColorImage::from_rgba_unmultiplied(
                        [w as usize, h as usize],
                        rgba.as_raw(),
                    );
                    self.thumbnail_handle = Some(ctx.load_texture(
                        "album_art",
                        color_image,
                        egui::TextureOptions::LINEAR,
                    ));
                }
            }
        }
    }

    fn set_dyn_color(&self, value: bool) {
        let mut locked = self.state.lock().unwrap();
        locked.settings.dynamic_color = value;
        save_settings(&locked.settings);
    }

    fn set_visualizer(&self, value: bool) {
        let mut locked = self.state.lock().unwrap();
        locked.settings.audio_visualizer = value;
        save_settings(&locked.settings);
    }
}

impl eframe::App for OsmvApp {
    fn update(&mut self, ctx: &Context, _frame: &mut eframe::Frame) {
        ctx.request_repaint_after(std::time::Duration::from_millis(1000));

        self.sync_state(ctx);
        self.pulse_t = (self.pulse_t + 0.03) % (2.0 * std::f32::consts::PI);

        // Full-window dark background
        egui::CentralPanel::default()
            .frame(egui::Frame::NONE.fill(BG_DARK))
            .show(ctx, |ui| {
                self.draw_main(ui);
            });
    }
}

impl OsmvApp {
    fn draw_main(&mut self, ui: &mut egui::Ui) {
        let avail = ui.available_rect_before_wrap();
        let card_rect = avail.shrink(16.0);

        // Shadow glow
        let painter = ui.painter();
        for i in 0..8u8 {
            let expand = (i as f32) * 2.5;
            let alpha = 30u8.saturating_sub(i * 3);
            painter.rect_filled(
                card_rect.expand(expand),
                14.0 + expand * 0.5,
                egui::Color32::from_rgba_premultiplied(99, 102, 241, alpha),
            );
        }
        painter.rect_filled(card_rect, 14.0, BG_PANEL);
        painter.rect_stroke(
            card_rect,
            14.0,
            egui::Stroke::new(1.0, egui::Color32::from_rgba_premultiplied(99, 102, 241, 80)),
            egui::StrokeKind::Middle,
        );

        let inner_rect = card_rect.shrink(16.0);
        let mut child_ui = ui.new_child(egui::UiBuilder::new().max_rect(inner_rect));
        child_ui.horizontal(|ui| {
            self.draw_album_art(ui);
            ui.add_space(16.0);
            ui.vertical(|ui| {
                self.draw_track_info(ui);
                ui.add_space(10.0);
                self.draw_status_badge(ui);
                ui.add_space(12.0);
                self.draw_toggles(ui);
            });
        });
    }

    fn draw_album_art(&self, ui: &mut egui::Ui) {
        let size = egui::vec2(100.0, 100.0);
        let (rect, _) = ui.allocate_exact_size(size, egui::Sense::hover());
        let painter = ui.painter();

        painter.rect_filled(rect, 10.0, egui::Color32::from_rgb(22, 22, 36));

        if let Some(handle) = &self.thumbnail_handle {
            painter.image(
                handle.id(),
                rect,
                egui::Rect::from_min_max(egui::pos2(0.0, 0.0), egui::pos2(1.0, 1.0)),
                egui::Color32::WHITE,
            );
            painter.rect_stroke(
                rect,
                10.0,
                egui::Stroke::new(1.5, egui::Color32::from_rgba_premultiplied(255, 255, 255, 30)),
                egui::StrokeKind::Middle,
            );
        } else {
            painter.rect_stroke(
                rect,
                10.0,
                egui::Stroke::new(1.5, egui::Color32::from_rgba_premultiplied(99, 102, 241, 60)),
                egui::StrokeKind::Middle,
            );
            painter.text(
                rect.center(),
                egui::Align2::CENTER_CENTER,
                "🎵",
                egui::FontId::proportional(32.0),
                egui::Color32::from_rgb(80, 80, 130),
            );
        }
    }

    fn draw_track_info(&self, ui: &mut egui::Ui) {
        ui.add(
            egui::Label::new(
                egui::RichText::new(&self.cached_title)
                    .color(TEXT_MAIN)
                    .font(egui::FontId::new(15.0, egui::FontFamily::Name("Inter-Bold".into())))
                    .strong(),
            )
            .wrap(),
        );

        if !self.cached_artist.is_empty() && self.cached_artist != "Unknown Artist" {
            ui.add(
                egui::Label::new(
                    egui::RichText::new(&self.cached_artist)
                        .color(TEXT_SUB)
                        .font(egui::FontId::proportional(12.5)),
                )
                .wrap(),
            );
        }

        if !self.cached_album.is_empty() {
            ui.add(
                egui::Label::new(
                    egui::RichText::new(format!("💿 {}", self.cached_album))
                        .color(TEXT_STATUS)
                        .font(egui::FontId::proportional(11.0)),
                )
                .wrap(),
            );
        }
    }

    fn draw_status_badge(&self, ui: &mut egui::Ui) {
        let (color, dot_color, label) = match self.cached_status.as_str() {
            "playing" => (
                egui::Color32::from_rgb(34, 197, 94),
                egui::Color32::from_rgb(74, 222, 128),
                "▶ Playing",
            ),
            "paused" => (
                egui::Color32::from_rgb(234, 179, 8),
                egui::Color32::from_rgb(253, 224, 71),
                "⏸ Paused",
            ),
            "stopped" => (
                egui::Color32::from_rgb(239, 68, 68),
                egui::Color32::from_rgb(252, 165, 165),
                "⏹ Stopped",
            ),
            _ => (TEXT_STATUS, TEXT_STATUS, "— Idle"),
        };

        ui.horizontal(|ui| {
            let dot_size = if self.cached_status == "playing" {
                let pulse = (self.pulse_t.sin() * 0.5 + 0.5) * 2.0 + 4.0;
                egui::vec2(pulse, pulse)
            } else {
                egui::vec2(6.0, 6.0)
            };

            let (dot_rect, _) = ui.allocate_exact_size(dot_size, egui::Sense::hover());
            ui.painter()
                .circle_filled(dot_rect.center(), dot_size.x / 2.0, dot_color);

            ui.add(egui::Label::new(
                egui::RichText::new(label)
                    .color(color)
                    .font(egui::FontId::proportional(11.5)),
            ));
        });
    }

    fn draw_toggles(&mut self, ui: &mut egui::Ui) {
        let mut dyn_color = self.cached_dyn_color;
        let changed_dc = ui
            .checkbox(
                &mut dyn_color,
                egui::RichText::new("🎨 Match cover color")
                    .color(TEXT_SUB)
                    .font(egui::FontId::proportional(11.5)),
            )
            .changed();
        if changed_dc {
            self.set_dyn_color(dyn_color);
            self.cached_dyn_color = dyn_color;
        }

        let mut visualizer = self.cached_visualizer;
        let changed_viz = ui
            .checkbox(
                &mut visualizer,
                egui::RichText::new("🎚 Audio visualizer (beta)")
                    .color(TEXT_SUB)
                    .font(egui::FontId::proportional(11.5)),
            )
            .changed();
        if changed_viz {
            self.set_visualizer(visualizer);
            self.cached_visualizer = visualizer;
        }
    }
}

fn setup_fonts(ctx: &Context) {
    let mut fonts = FontDefinitions::default();
    fonts.font_data.insert(
        "Inter".into(),
        FontData::from_static(INTER_FONT).into(),
    );
    fonts.font_data.insert(
        "Inter-Bold".into(),
        FontData::from_static(INTER_BOLD).into(),
    );
    fonts
        .families
        .entry(FontFamily::Proportional)
        .or_default()
        .insert(0, "Inter".into());
    fonts.families.insert(
        FontFamily::Name("Inter-Bold".into()),
        vec!["Inter-Bold".into()],
    );
    ctx.set_fonts(fonts);
}

fn setup_visuals(ctx: &Context) {
    let mut visuals = egui::Visuals::dark();
    visuals.window_fill = BG_DARK;
    visuals.panel_fill = BG_DARK;
    visuals.faint_bg_color = BG_PANEL;
    visuals.override_text_color = Some(TEXT_MAIN);
    visuals.widgets.inactive.bg_fill = egui::Color32::from_rgb(30, 30, 50);
    visuals.widgets.inactive.fg_stroke = egui::Stroke::new(1.0, TEXT_STATUS);
    visuals.widgets.active.bg_fill = ACCENT;
    visuals.widgets.active.fg_stroke = egui::Stroke::new(1.5, egui::Color32::WHITE);
    visuals.widgets.hovered.bg_fill = ACCENT_DIM;
    visuals.widgets.hovered.fg_stroke = egui::Stroke::new(1.5, TEXT_MAIN);
    visuals.selection.bg_fill = ACCENT;
    ctx.set_visuals(visuals);
}
