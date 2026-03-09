"""
Discord Rich Presence service for OSMV Linux.
Mirrors the logic of DiscordRpcService.cs (Windows version).

Requires: pypresence  (pip install pypresence)
"""

import threading
import time

try:
    from pypresence import Presence
    HAS_PYPRESENCE = True
except ImportError:
    HAS_PYPRESENCE = False
    print("[DiscordRPC] pypresence not installed. Discord RPC will not work.")


class DiscordRpcService:
    CLEAR_DELAY = 5.0   # seconds before clearing presence on pause/stop

    def __init__(self):
        self._rpc: Presence | None = None
        self._initialized = False
        self._clear_timer: threading.Timer | None = None

    # ── Public API ────────────────────────────────────────────────────────────
    def initialize(self, client_id: str) -> None:
        if self._initialized:
            return
        if not HAS_PYPRESENCE:
            return
        if not client_id or not client_id.strip():
            return
        try:
            self._rpc = Presence(client_id.strip())
            self._rpc.connect()
            self._initialized = True
            print(f"[DiscordRPC] Connected with client_id={client_id}")
        except Exception as e:
            print(f"[DiscordRPC] Initialization error: {e}")
            self._rpc = None

    def update_presence(self, title: str, artist: str, is_playing: bool,
                        cover_url: str = None) -> None:
        if not self._initialized or self._rpc is None:
            return
        try:
            if not title and not artist:
                self._schedule_clear()
                return
            if not is_playing:
                self._schedule_clear()
                return
            else:
                self._cancel_clear()

            large_image = cover_url if cover_url else "placeholder"

            self._rpc.update(
                details=title,
                state=f"by {artist}",
                large_image=large_image,
                large_text=f"{title} - {artist}",
                small_image="osmv_logo",
                small_text="OBS Stream Music Viewer",
                buttons=[{"label": "Website",
                          "url": "https://streammusicviewer.github.io/site/"}],
            )
        except Exception as e:
            print(f"[DiscordRPC] UpdatePresence error: {e}")

    def clear_presence(self) -> None:
        if not self._initialized or self._rpc is None:
            return
        try:
            self._rpc.clear()
        except Exception:
            pass

    def dispose(self) -> None:
        self._cancel_clear()
        self.clear_presence()
        if self._rpc is not None:
            try:
                self._rpc.close()
            except Exception:
                pass
            self._rpc = None
        self._initialized = False

    # ── Internals ─────────────────────────────────────────────────────────────
    def _schedule_clear(self) -> None:
        if self._clear_timer is not None:
            return
        self._clear_timer = threading.Timer(self.CLEAR_DELAY, self._do_clear)
        self._clear_timer.daemon = True
        self._clear_timer.start()

    def _cancel_clear(self) -> None:
        if self._clear_timer is not None:
            self._clear_timer.cancel()
            self._clear_timer = None

    def _do_clear(self) -> None:
        self._clear_timer = None
        self.clear_presence()
