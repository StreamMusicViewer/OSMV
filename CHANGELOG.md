# Changelog

All notable changes to OSMV are documented in this file.

## v5.1.0 — OBS Overlay Studio, Automatic Twitch OAuth & Linux Wayland `app_id`

### OBS Overlay Studio & Previews
- **OBS Overlay Studio (`Now Playing`)**: Replaced the basic dynamic color / visualizer toggles with a complete 5-tab overlay customizer (`Presets & Style`, `Layout & Content`, `Visualizer`, `Custom CSS`, `Snaz Exports`).
- **Live Stage & Browser Previews**: Added an embedded Dear ImGui **Live OBS Stage Preview** (`OBS Checker` / `Dark` / `Light`, `Live Track` / `Demo Track`) alongside a standalone `shared/preview.html` (**Browser Preview** with `Checker` / `Dark` / `Light` and `Demo / Idle`).
- **Visual Presets & Multi-Scene OBS**: Added 7 built-in presets (`classic`, `glass`, `neon`, `compact`, `vinyl`, `minimal`, `banner`), **Reset to Defaults**, and **Saved Presets (Multi-Scene OBS)** stored in `obsCustomPresets` inside `shared/settings.json` (**Save Preset**, **Update**, **Delete**, and **Scene URL** for `?preset=<Name>` overlays).
- **Layout, Typography & Motion**: Added 5 canvas anchors, 4 layout directions (`horizontal`, `horizontal-right`, `vertical`, `compact`), background gradients, backdrop blur, borders/shadows/glow, 3 **Album color target** modes (`card`, `accent`, `subtle` exposing `--osmv-album-hue` and `--osmv-album-rgb`), **Vinyl spin while playing**, Google Fonts support, marquee scrolling, 5 transitions (`fade`, `slide-up`, `slide-left`, `zoom`, `none`), hide-when-idle timers, and custom templates (`$song`, `$artist`, `$album`, `$player`, `$status`, and uppercase `*_upper` tokens).
- **5-Style Audio Visualizer**: Added `bars`, `mirrored`, `dots`, `wave`, and `blocks` visualizers with `top`, `bottom`, and `background` placement, `album` / `custom` gradient / `rainbow` color modes, and configurable bar count (`8..48`), height, gap, and energy.
- **Custom CSS Editor**: Added an auto-sizing monospace CSS editor with syntax highlighting, line numbers, `Ln/Col` cursor bar, **Load Studio CSS into Editor**, **Copy Generated CSS**, selector snippet chips, layered **Enable custom CSS**, and **Raw CSS only (disable generated Studio styles)**.
- **CORS-Safe `file://` Delivery (`current_song.js` + `widget_config.js`)**: Daemon and settings export `shared/current_song.js` (`window.__OSMV_CURRENT_SONG`) and `shared/widget_config.js` (`window.__OSMV_WIDGET_CONFIG`, containing built-in and custom `presets` maps) alongside JSON files, plus `"player"` in `current_song.json`, so `?preset=<Name>` Browser Sources work cleanly in OBS without `file://` CORS failures.

### Twitch Helix & Formatters
- **Automatic Localhost OAuth**: Clicking **Get OAuth Token** opens Twitch authorization and runs a temporary local HTTP listener on `http://localhost` (port `80`, `180` seconds timeout) to capture `#access_token` and fill/save credentials automatically (`src/gui/twitch_oauth.cpp`).
- **Token Extraction & Status**: Added `fmt::extract_twitch_token()` so pasting a full localhost redirect URL, `oauth:...`, or raw token works automatically, with explicit inline OAuth/client error feedback.

### Linux Desktop Integration & Window Identification
- **Wayland `app_id` & X11 `WM_CLASS`**: Exposed `app_id="io.github.streammusicviewer.OSMV"` on Wayland and `WM_CLASS` (`instance="osmv"`, `class="OSMV"`) on X11/XWayland (`backend_linux_glfw.cpp`) for window and workspace rules (Sway, Hyprland, i3).
- **Wayland Icon Fix & GLFW 3.4 Fallback**: Avoided `GLFW_PLATFORM_ERROR` from `glfwSetWindowIcon` under Wayland and added CMake FetchContent fallback to GLFW 3.4 when system GLFW is `< 3.4` and Wayland headers are installed.
- **Desktop Entry & Icon**: Added `linux/io.github.streammusicviewer.OSMV.desktop` and `linux/io.github.streammusicviewer.OSMV.png` with CMake install rules.

### Build, Versioning & Documentation
- **OBS-Safe Dynamic `.txt` Clearing**: Disabling any module or text export now empties existing `.txt` files (`TrackArtist.txt`, `TrackSong.txt`, `TrackAlbum.txt`, `PlayingNow.txt`, `Time.txt`, `Date.txt`, `Countdown.txt`, `ChronoDown.txt`, `ChronoUp.txt`, `SystemInfo.txt`, `TextlineChanger.txt`, `TwitchViewers.txt`, and disabled Dynamic Files text targets) instead of deleting them, keeping OBS Text sources working cleanly.
- **Single-Source Versioning & Windows File Metadata**: Propagated `PROJECT_VERSION` (`5.1.0`) from `CMakeLists.txt` via generated `common/version.hpp` across HTTP User-Agent, GUI header/About card, and `VS_VERSION_INFO` in `windows/OSMV.rc`.
- **Clean Windows Builds**: Updated `windows/build.bat` to pass `--clean-first`, preventing stale object files when headers change under localized MSVC.
- **License & English Docs**: Switched license to **GNU General Public License v3.0 (`GPL-3.0`)**, translated `README.md` and `TROUBLESHOOTING.md` to English, expanded `.gitignore`, and updated the Nextra docs in `docs/`.

---

## v5.0.0 — C++20 Rewrite & Snaz Modules

- Complete native **C++20** rewrite replacing `v4.0.0` with a dual-process low-RAM daemon (`OSMV.exe` on Windows with delay-loaded DX11/DWM; split `osmv` + `osmv-gui` on Linux).
- Redesigned **Dear ImGui** configuration interface with EN/FR localization, Dark/Light themes, and headless tray mode.
- Added Snaz-inspired modules: Time, Date, Countdown, Chrono Down/Up, System Info, Textline Changer, Dynamic Files, and Twitch Helix, plus native Discord IPC Listening status (`type: 2`).

