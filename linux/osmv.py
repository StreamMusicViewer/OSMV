#!/usr/bin/env python3
"""
OBS Stream Music Viewer — Linux Edition
Uses MPRIS2 via playerctl to detect playing media, then writes
current_song.json for the OBS browser source widget.

Requirements: see requirements.txt
"""

import base64
import json
import os
import subprocess
import sys
import threading
import time
import urllib.request
import urllib.parse
import tkinter as tk
from tkinter import ttk
from pathlib import Path

try:
    import pystray
    from pystray import MenuItem as item
    from PIL import Image, ImageDraw
    HAS_TRAY = True
except ImportError:
    HAS_TRAY = False
    print("[OSMV] pystray/Pillow not found — running without system tray.")

try:
    from discord_rpc_service import DiscordRpcService
    HAS_DISCORD = True
except ImportError:
    HAS_DISCORD = False
    print("[OSMV] discord_rpc_service not found — Discord RPC disabled.")

# ── Paths ─────────────────────────────────────────────────────────────────────
BASE_DIR         = Path(__file__).resolve().parent
OUTPUT_JSON      = BASE_DIR / "current_song.json"
SETTINGS_FILE    = BASE_DIR / "settings.json"
LOGO_PATH        = BASE_DIR / "OSMV_logo.png"   # PNG fallback logo for tray

# ── Globals ───────────────────────────────────────────────────────────────────
_app = None   # OSMVApp singleton


# ═══════════════════════════════════════════════════════════════════════════════
#  PlayerctlBackend — reads media metadata via the `playerctl` CLI tool
# ═══════════════════════════════════════════════════════════════════════════════
class PlayerctlBackend:
    """
    Thin wrapper around `playerctl` to retrieve MPRIS2 metadata.
    Falls back gracefully if playerctl is not installed.
    """

    def _run(self, *args) -> str:
        """Run `playerctl <args>` and return stdout, or '' on error."""
        try:
            result = subprocess.run(
                ["playerctl"] + list(args),
                capture_output=True,
                text=True,
                timeout=2
            )
            return result.stdout.strip()
        except FileNotFoundError:
            return "__NOT_FOUND__"
        except subprocess.TimeoutExpired:
            return ""
        except Exception:
            return ""

    def is_available(self) -> bool:
        out = self._run("--version")
        if out == "__NOT_FOUND__":
            return False
        return bool(out)

    def get_status(self) -> str:
        """Returns 'Playing', 'Paused', 'Stopped', or 'closed'."""
        s = self._run("status")
        if s in ("__NOT_FOUND__", "", "No players found"):
            return "closed"
        return s

    def get_metadata(self) -> dict:
        """Returns {title, artist, album, art_url} or empty dict."""
        data = {}
        try:
            title   = self._run("metadata", "title")
            artist  = self._run("metadata", "artist")
            album   = self._run("metadata", "album")
            art_url = self._run("metadata", "mpris:artUrl")

            if title and title not in ("__NOT_FOUND__", "No players found"):
                data["title"] = title
            if artist and artist not in ("__NOT_FOUND__", "No players found"):
                data["artist"] = artist
            if album and album not in ("__NOT_FOUND__", "No players found"):
                data["album"] = album
            if art_url and art_url not in ("__NOT_FOUND__", "No players found"):
                data["art_url"] = art_url
        except Exception:
            pass
        return data


# ═══════════════════════════════════════════════════════════════════════════════
#  Thumbnail helpers
# ═══════════════════════════════════════════════════════════════════════════════
def _load_thumbnail_as_base64(art_url: str) -> str | None:
    """
    Given an MPRIS artUrl (file:/// or http/https), return base64-encoded JPEG.
    """
    if not art_url:
        return None
    try:
        if art_url.startswith("file://"):
            path = urllib.request.url2pathname(art_url[7:])
            with open(path, "rb") as f:
                return base64.b64encode(f.read()).decode("ascii")
        else:
            req = urllib.request.Request(art_url, headers={"User-Agent": "OSMV/1.0"})
            with urllib.request.urlopen(req, timeout=5) as r:
                return base64.b64encode(r.read()).decode("ascii")
    except Exception as e:
        print(f"[OSMV] Thumbnail fetch error: {e}")
        return None


# ═══════════════════════════════════════════════════════════════════════════════
#  Settings
# ═══════════════════════════════════════════════════════════════════════════════
class Settings:
    def __init__(self):
        self.dynamic_color     = False
        self.audio_visualizer  = False
        self.discord_rpc       = False
        self.discord_client_id = "1479531788731809913"
        self.load()

    def load(self):
        try:
            if SETTINGS_FILE.exists():
                data = json.loads(SETTINGS_FILE.read_text())
                self.dynamic_color     = data.get("dynamicColor",    False)
                self.audio_visualizer  = data.get("audioVisualizer", False)
                self.discord_rpc       = data.get("discordRpc",      False)
                self.discord_client_id = data.get("discordClientId", self.discord_client_id)
        except Exception:
            pass

    def save(self):
        try:
            data = {
                "dynamicColor":    self.dynamic_color,
                "audioVisualizer": self.audio_visualizer,
                "discordRpc":      self.discord_rpc,
                "discordClientId": self.discord_client_id,
            }
            SETTINGS_FILE.write_text(json.dumps(data, indent=2))
        except Exception as e:
            print(f"[OSMV] Settings save error: {e}")


# ═══════════════════════════════════════════════════════════════════════════════
#  SettingsWindow — tkinter UI (mirrors the WPF window)
# ═══════════════════════════════════════════════════════════════════════════════
class SettingsWindow:
    def __init__(self, app):
        self.app = app
        self.root = None

    def show(self):
        if self.root and self.root.winfo_exists():
            self.root.deiconify()
            self.root.lift()
            return
        self._build()

    def _build(self):
        root = tk.Tk()
        self.root = root
        root.title("OBS Stream Music Viewer")
        root.resizable(False, False)
        root.configure(bg="#18181A")
        root.geometry("420x240")
        root.protocol("WM_DELETE_WINDOW", self._on_close)

        # Try to set icon
        if LOGO_PATH.exists():
            try:
                icon_img = tk.PhotoImage(file=str(LOGO_PATH))
                root.iconphoto(True, icon_img)
            except Exception:
                pass

        # ── Style
        style = ttk.Style(root)
        style.theme_use("clam")
        style.configure("TCheckbutton",
                         background="#18181A", foreground="#B0B0B0",
                         focuscolor="#18181A", font=("Segoe UI", 11))
        style.configure("TLabel",
                         background="#18181A", foreground="#B0B0B0",
                         font=("Segoe UI", 11))
        style.configure("TEntry",
                         fieldbackground="#28282A", foreground="#B0B0B0",
                         insertcolor="#B0B0B0", font=("Segoe UI", 11))
        style.configure("Error.TLabel",
                         foreground="red", background="#18181A",
                         font=("Segoe UI", 9))

        # ── Left: current song display
        left = tk.Frame(root, bg="#18181A", width=100, height=100)
        left.pack_propagate(False)
        left.pack(side=tk.LEFT, padx=10, pady=10)

        self._art_label = tk.Label(left, bg="#282830", width=9, height=5, text="🎵",
                                   font=("Segoe UI", 24), fg="#5050A0")
        self._art_label.pack(expand=True)

        # ── Right: info + options
        right = tk.Frame(root, bg="#18181A")
        right.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 10), pady=10)

        self._title_var  = tk.StringVar(value="Waiting for music...")
        self._artist_var = tk.StringVar(value="---")
        self._status_var = tk.StringVar(value="Status: unknown")

        tk.Label(right, textvariable=self._title_var,  bg="#18181A", fg="white",
                 font=("Segoe UI", 13, "bold"), anchor="w").pack(fill=tk.X)
        tk.Label(right, textvariable=self._artist_var, bg="#18181A", fg="#B0B0B0",
                 font=("Segoe UI", 11), anchor="w").pack(fill=tk.X, pady=(2, 0))
        tk.Label(right, textvariable=self._status_var, bg="#18181A", fg="#606060",
                 font=("Segoe UI", 10), anchor="w").pack(fill=tk.X, pady=(4, 6))

        # Dynamic color checkbox
        s = self.app.settings
        self._dyncolor_var = tk.BooleanVar(value=s.dynamic_color)
        ttk.Checkbutton(right, text="🎨 Match cover color",
                        variable=self._dyncolor_var,
                        command=self._on_dyncolor).pack(anchor="w")

        # Audio visualizer checkbox
        self._viz_var = tk.BooleanVar(value=s.audio_visualizer)
        ttk.Checkbutton(right, text="🎚 Audio visualizer (beta)",
                        variable=self._viz_var,
                        command=self._on_viz).pack(anchor="w", pady=(2, 0))

        # Discord RPC checkbox
        if HAS_DISCORD:
            self._rpc_var = tk.BooleanVar(value=s.discord_rpc)
            ttk.Checkbutton(right, text="🎮 Discord Rich Presence",
                            variable=self._rpc_var,
                            command=self._on_rpc).pack(anchor="w", pady=(4, 0))

            discord_row = tk.Frame(right, bg="#18181A")
            discord_row.pack(anchor="w", padx=(18, 0))

            tk.Label(discord_row, text="Client ID:", bg="#18181A", fg="#606060",
                     font=("Segoe UI", 9)).pack(side=tk.LEFT)

            self._cid_var = tk.StringVar(value=s.discord_client_id)
            cid_entry = ttk.Entry(discord_row, textvariable=self._cid_var, width=20)
            cid_entry.pack(side=tk.LEFT, padx=(4, 0))
            cid_entry.bind("<FocusOut>", self._on_cid_changed)

            self._discord_row = discord_row
            self._toggle_discord_row()
        else:
            ttk.Label(right, text="🎮 Discord RPC (install pypresence)",
                      style="TLabel").pack(anchor="w", pady=(4, 0))

        # Error label
        self._error_var = tk.StringVar()
        ttk.Label(right, textvariable=self._error_var,
                  style="Error.TLabel").pack(anchor="w", pady=(4, 0))

        root.mainloop()

    def _on_close(self):
        if self.root:
            self.root.withdraw()

    # ── Checkbox handlers
    def _on_dyncolor(self):
        self.app.settings.dynamic_color = self._dyncolor_var.get()
        self.app.settings.save()

    def _on_viz(self):
        self.app.settings.audio_visualizer = self._viz_var.get()
        self.app.settings.save()

    def _on_rpc(self):
        self.app.settings.discord_rpc = self._rpc_var.get()
        self.app.settings.save()
        self._toggle_discord_row()
        self.app._sync_discord_rpc()

    def _on_cid_changed(self, event=None):
        new_id = self._cid_var.get()
        if new_id != self.app.settings.discord_client_id:
            self.app.settings.discord_client_id = new_id
            self.app.settings.save()
            if self.app.settings.discord_rpc:
                self.app._reinit_discord_rpc()

    def _toggle_discord_row(self):
        if hasattr(self, "_discord_row"):
            if self._rpc_var.get():
                self._discord_row.pack(anchor="w", padx=(18, 0))
            else:
                self._discord_row.pack_forget()

    # ── Called by the polling thread to update display
    def update_song(self, title: str, artist: str, status: str):
        if self.root and self.root.winfo_exists():
            self.root.after(0, lambda: self._do_update(title, artist, status))

    def _do_update(self, title, artist, status):
        self._title_var.set(title or "Waiting for music...")
        self._artist_var.set(artist or "---")
        self._status_var.set(f"Status: {status}")

    def set_error(self, msg: str):
        if self.root and self.root.winfo_exists():
            self.root.after(0, lambda: self._error_var.set(msg))


# ═══════════════════════════════════════════════════════════════════════════════
#  OSMVApp — main orchestrator
# ═══════════════════════════════════════════════════════════════════════════════
class OSMVApp:
    POLL_INTERVAL = 1.0  # seconds

    def __init__(self):
        self.settings    = Settings()
        self.backend     = PlayerctlBackend()
        self.ui          = SettingsWindow(self)
        self.tray        = None
        self._running    = True
        self._poll_thread = None

        # Discord RPC
        self._discord: DiscordRpcService | None = None
        self._last_discord_title  = None
        self._last_discord_artist = None
        self._current_cover_url   = None

        # State
        self._last_json_state = None

    # ── Discord helpers ───────────────────────────────────────────────────────
    def _sync_discord_rpc(self):
        if self.settings.discord_rpc and HAS_DISCORD:
            if not self._discord:
                self._discord = DiscordRpcService()
                self._discord.initialize(self.settings.discord_client_id)
        else:
            if self._discord:
                self._discord.clear_presence()
                self._discord.dispose()
                self._discord = None

    def _reinit_discord_rpc(self):
        if self._discord:
            self._discord.clear_presence()
            self._discord.dispose()
            self._discord = None
        if self.settings.discord_rpc and HAS_DISCORD and self.settings.discord_client_id:
            self._discord = DiscordRpcService()
            self._discord.initialize(self.settings.discord_client_id)

    def _update_discord(self, title, artist, is_playing):
        if not self.settings.discord_rpc or not self._discord:
            return
        if title != self._last_discord_title or artist != self._last_discord_artist:
            self._last_discord_title  = title
            self._last_discord_artist = artist
            self._current_cover_url   = None
            if title:
                # Fetch iTunes cover asynchronously
                threading.Thread(
                    target=self._fetch_cover, args=(title, artist), daemon=True
                ).start()
        self._discord.update_presence(title, artist, is_playing, self._current_cover_url)

    def _fetch_cover(self, title, artist):
        try:
            query = urllib.parse.quote(f"{artist} {title}")
            url = f"https://itunes.apple.com/search?term={query}&entity=song&limit=1"
            req = urllib.request.Request(url, headers={"User-Agent": "OSMV/1.0"})
            with urllib.request.urlopen(req, timeout=8) as r:
                data = json.loads(r.read())
            results = data.get("results", [])
            if results:
                artwork = results[0].get("artworkUrl100", "")
                self._current_cover_url = artwork.replace("100x100bb", "512x512bb")
        except Exception:
            pass

    # ── JSON output ───────────────────────────────────────────────────────────
    def _write_json(self, title, artist, album, status, thumbnail_b64):
        try:
            if not title and not artist:
                data = None
            else:
                data = {
                    "title":          title,
                    "artist":         artist,
                    "album":          album,
                    "thumbnail":      thumbnail_b64,
                    "status":         status,
                    "dynamicColor":   self.settings.dynamic_color,
                    "audioVisualizer": self.settings.audio_visualizer,
                    "timestamp":      time.strftime("%Y-%m-%dT%H:%M:%S"),
                }
            OUTPUT_JSON.write_text(json.dumps(data, indent=2))
        except Exception as e:
            print(f"[OSMV] JSON write error: {e}")

    # ── Polling loop ──────────────────────────────────────────────────────────
    def _poll_loop(self):
        if not self.backend.is_available():
            msg = "playerctl not found. Install it with: sudo apt install playerctl"
            print(f"[OSMV] WARNING: {msg}")
            self.ui.set_error(msg)

        while self._running:
            try:
                self._tick()
            except Exception as e:
                print(f"[OSMV] Poll error: {e}")
            time.sleep(self.POLL_INTERVAL)

    def _tick(self):
        status_raw = self.backend.get_status()   # Playing / Paused / Stopped / closed
        status = status_raw.lower()

        if status in ("closed", "no players found", ""):
            self._write_json(None, None, None, "closed", None)
            self.ui.update_song("", "", "closed")
            self._update_discord(None, None, False)
            return

        meta = self.backend.get_metadata()
        title  = meta.get("title", "Unknown Title")
        artist = meta.get("artist", "Unknown Artist")
        album  = meta.get("album", "")
        art_url = meta.get("art_url", "")

        is_playing = (status == "playing")

        # Only reload thumbnail when song changes to avoid re-fetching every second
        state_key = (title, artist, art_url)
        if state_key != self._last_json_state:
            self._last_json_state = state_key
            thumbnail_b64 = _load_thumbnail_as_base64(art_url)
        else:
            # Re-read current json to preserve existing thumbnail
            try:
                existing = json.loads(OUTPUT_JSON.read_text())
                thumbnail_b64 = existing.get("thumbnail") if existing else None
            except Exception:
                thumbnail_b64 = None

        self._write_json(title, artist, album, status, thumbnail_b64)
        self.ui.update_song(title, artist, status)

        if HAS_DISCORD:
            self._update_discord(title, artist, is_playing)

    # ── System tray ───────────────────────────────────────────────────────────
    def _make_tray_icon(self):
        """Create a simple tray icon image if OSMV_logo.png is not present."""
        if LOGO_PATH.exists():
            try:
                return Image.open(LOGO_PATH).resize((64, 64))
            except Exception:
                pass
        # Generate a simple purple circle as fallback
        img = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        draw.ellipse([4, 4, 60, 60], fill=(100, 60, 200, 255))
        draw.text((22, 20), "♫", fill="white")
        return img

    def _run_tray(self):
        if not HAS_TRAY:
            return

        def on_show(icon, item):
            threading.Thread(target=self.ui.show, daemon=True).start()

        def on_quit(icon, item):
            self._running = False
            icon.stop()

        menu = pystray.Menu(
            item("Show", on_show, default=True),
            pystray.Menu.SEPARATOR,
            item("Quit", on_quit),
        )

        icon_img = self._make_tray_icon()
        self.tray = pystray.Icon(
            "OSMV",
            icon_img,
            "OBS Stream Music Viewer",
            menu
        )
        self.tray.run()

    # ── Entry point ───────────────────────────────────────────────────────────
    def run(self):
        # Start Discord RPC if enabled in settings
        self._sync_discord_rpc()

        # Start polling in background thread
        self._poll_thread = threading.Thread(target=self._poll_loop, daemon=True)
        self._poll_thread.start()

        if HAS_TRAY:
            # Run tray in a background thread, show settings window in main thread
            tray_thread = threading.Thread(target=self._run_tray, daemon=True)
            tray_thread.start()
            # Show settings window immediately on launch
            self.ui.show()   # blocks (tk.mainloop) until window closes
            # After window closes, stay running (icon still showing)
            # Keep process alive while tray is active
            while self._running:
                time.sleep(0.5)
        else:
            # No tray: show window directly (blocking)
            threading.Thread(target=self.ui.show, daemon=False).start()
            while self._running:
                time.sleep(0.5)

        # Cleanup
        self._cleanup()

    def _cleanup(self):
        print("[OSMV] Shutting down...")
        self._running = False
        if self._discord:
            self._discord.clear_presence()
            self._discord.dispose()
        try:
            self._write_json(None, None, None, "closed", None)
        except Exception:
            pass


# ═══════════════════════════════════════════════════════════════════════════════
#  Entrypoint
# ═══════════════════════════════════════════════════════════════════════════════
if __name__ == "__main__":
    # Single-instance check (Linux: use lock file)
    lock_file = BASE_DIR / ".osmv.lock"
    try:
        import fcntl
        _lock_fd = open(lock_file, "w")
        fcntl.flock(_lock_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        print("[OSMV] Another instance is already running.")
        sys.exit(1)
    except Exception:
        pass   # fcntl not available (non-Linux), continue anyway

    _app = OSMVApp()
    _app.run()
