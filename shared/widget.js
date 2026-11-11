(() => {
    'use strict';

    const DEFAULT_CONFIG = {
        preset: 'classic',
        position: 'bottom-right',
        layout: 'horizontal',
        widthPx: 450,
        paddingPx: 20,
        gapPx: 16,
        borderRadiusPx: 16,
        marginPx: 20,
        showAlbumArt: true,
        artSizePx: 80,
        artRadiusPx: 8,
        spinArtwork: false,
        showTitle: true,
        showArtist: true,
        showAlbum: false,
        showStatusBadge: false,
        titleFormat: '$song',
        artistFormat: '$artist',
        albumFormat: '$album',
        badgeFormat: '$status_upper',
        idleText: 'Nothing currently playing',
        marqueeScroll: true,
        scrollSpeed: 1.0,
        dynamicColor: false,
        dynamicColorMode: 'card',
        audioVisualizer: false,
        vizPosition: 'top',
        vizStyle: 'bars',
        vizBarCount: 20,
        vizHeightPx: 56,
        vizGapPx: 4,
        vizColorMode: 'album',
        vizColorTopCss: 'rgba(155, 123, 255, 1.000)',
        vizColorBottomCss: 'rgba(76, 51, 179, 1.000)',
        accentColorCss: 'rgba(124, 92, 255, 1.000)',
        vizSpeed: 1.0,
        fontFamily: 'Segoe UI',
        loadGoogleFont: false,
        hideWhenIdle: true,
        hideDelaySec: 10,
        transitionStyle: 'fade',
        customCssEnabled: false,
        customCssRawOverride: false,
        customCss: '',
        generatedStudioCss: ''
    };

    const SYSTEM_FONTS = new Set([
        'segoe ui', 'arial', 'helvetica', 'verdana', 'tahoma', 'trebuchet ms',
        'georgia', 'times new roman', 'courier new', 'consolas', 'system-ui', 'sans-serif', 'monospace'
    ]);

    function makeDemoArtworkDataUri() {
        const svg =
            `<svg xmlns="http://www.w3.org/2000/svg" width="160" height="160" viewBox="0 0 160 160">` +
            `<defs>` +
            `<linearGradient id="g" x1="0" y1="0" x2="1" y2="1">` +
            `<stop offset="0%" stop-color="#ff3d8b"/>` +
            `<stop offset="50%" stop-color="#7c5cff"/>` +
            `<stop offset="100%" stop-color="#00d4ff"/>` +
            `</linearGradient>` +
            `</defs>` +
            `<rect width="160" height="160" fill="url(#g)"/>` +
            `<circle cx="80" cy="80" r="46" fill="none" stroke="rgba(255,255,255,0.45)" stroke-width="5"/>` +
            `<circle cx="80" cy="80" r="14" fill="rgba(16,18,28,0.85)"/>` +
            `</svg>`;
        return 'data:image/svg+xml;charset=utf-8,' + encodeURIComponent(svg);
    }

    const DEMO_SONG = {
        title: 'Midnight City',
        artist: 'M83',
        album: "Hurry Up, We're Dreaming",
        player: 'Spotify',
        status: 'playing',
        thumbnailDataUri: makeDemoArtworkDataUri()
    };

    function parseCssColorToHue(cssColor) {
        const m = /rgba?\((\d+),\s*(\d+),\s*(\d+)/i.exec(cssColor || '');
        if (!m) return 260;
        let r = Number(m[1]) / 255;
        let g = Number(m[2]) / 255;
        let b = Number(m[3]) / 255;
        const max = Math.max(r, g, b);
        const min = Math.min(r, g, b);
        if (max === min) return 260;
        const d = max - min;
        let h = 0;
        if (max === r) h = ((g - b) / d + (g < b ? 6 : 0)) / 6;
        else if (max === g) h = ((b - r) / d + 2) / 6;
        else h = ((r - g) / d + 4) / 6;
        return h * 360;
    }

    function formatTokens(pattern, song) {
        const title = song.title || '';
        const artist = song.artist || '';
        const album = song.album || '';
        const player = song.player || 'Media';
        const status = song.status || 'idle';
        return String(pattern || '')
            .replace(/\$song_upper/g, title.toUpperCase())
            .replace(/\$artist_upper/g, artist.toUpperCase())
            .replace(/\$album_upper/g, album.toUpperCase())
            .replace(/\$status_upper/g, status.toUpperCase())
            .replace(/\$song/g, title)
            .replace(/\$artist/g, artist)
            .replace(/\$album/g, album)
            .replace(/\$player/g, player)
            .replace(/\$status/g, status);
    }

    class AudioVisualizer {
        constructor() {
            this.topWrapper = document.getElementById('visualizer-top-wrapper');
            this.topCanvas = document.getElementById('visualizer-top');
            this.bottomWrapper = document.getElementById('visualizer-bottom-wrapper');
            this.bottomCanvas = document.getElementById('visualizer-bottom');
            this.bgCanvas = document.getElementById('visualizer-bg');

            this.position = 'top';
            this.style = 'bars';
            this.barCount = 20;
            this.heightPx = 56;
            this.gapPx = 4;
            this.colorMode = 'album';
            this.colorTop = 'rgba(155, 123, 255, 1)';
            this.colorBottom = 'rgba(76, 51, 179, 1)';
            this.speed = 1.0;
            this.hue = 260;

            this._enabled = false;
            this._animFrame = null;
            this._updateTimer = null;
            this._current = new Float32Array(64).fill(0);
            this._target = new Float32Array(64).fill(0);
            this._phase = 0;

            this._ro = new ResizeObserver(() => this._syncCanvasSize());
            this._ro.observe(this.topWrapper);
            this._ro.observe(this.bottomWrapper);
            this._ro.observe(this.bgCanvas);
        }

        configure(cfg) {
            const newPos = cfg.vizPosition || 'top';
            const newCount = Math.max(6, Math.min(48, Number(cfg.vizBarCount) || 20));
            const newSpeed = Math.max(0.4, Math.min(2.0, Number(cfg.vizSpeed) || 1.0));
            const speedChanged = Math.abs(newSpeed - this.speed) > 0.02;

            this.position = newPos;
            this.style = cfg.vizStyle || 'bars';
            this.barCount = newCount;
            this.heightPx = Math.max(16, Math.min(160, Number(cfg.vizHeightPx) || 56));
            this.gapPx = Math.max(1, Math.min(12, Number(cfg.vizGapPx) || 4));
            this.colorMode = cfg.vizColorMode || 'album';
            this.colorTop = cfg.vizColorTopCss || this.colorTop;
            this.colorBottom = cfg.vizColorBottomCss || this.colorBottom;
            this.speed = newSpeed;

            this.topWrapper.style.height = this.heightPx + 'px';
            this.bottomWrapper.style.height = this.heightPx + 'px';

            if (this._enabled) {
                this._applyWrapperVisibility();
                if (speedChanged) this._startTargetLoop();
            }
            this._syncCanvasSize();
        }

        _activeCanvas() {
            if (this.position === 'bottom') return this.bottomCanvas;
            if (this.position === 'background') return this.bgCanvas;
            return this.topCanvas;
        }

        _syncCanvasSize() {
            const c = this._activeCanvas();
            if (!c) return;
            const w = c.clientWidth || (this.position === 'background' ? 450 : c.parentElement.offsetWidth);
            const h = c.clientHeight || this.heightPx;
            if (w > 0 && h > 0 && (c.width !== w || c.height !== h)) {
                c.width = w;
                c.height = h;
            }
        }

        _applyWrapperVisibility() {
            this.topWrapper.classList.toggle('visualizer-hidden', !this._enabled || this.position !== 'top');
            this.bottomWrapper.classList.toggle('visualizer-hidden', !this._enabled || this.position !== 'bottom');
            this.bgCanvas.classList.toggle('visualizer-bg-active', this._enabled && this.position === 'background');
            requestAnimationFrame(() => this._syncCanvasSize());
        }

        setHue(h) {
            this.hue = h;
        }

        enable() {
            if (this._enabled) return;
            this._enabled = true;
            this._applyWrapperVisibility();
            this._startTargetLoop();
            this._draw();
        }

        disable() {
            if (!this._enabled) return;
            this._enabled = false;
            this._applyWrapperVisibility();
            if (this._animFrame) {
                cancelAnimationFrame(this._animFrame);
                this._animFrame = null;
            }
            if (this._updateTimer) {
                clearInterval(this._updateTimer);
                this._updateTimer = null;
            }
            this._current.fill(0);
            this._target.fill(0);
        }

        _startTargetLoop() {
            if (this._updateTimer) clearInterval(this._updateTimer);
            const intervalMs = Math.round(160 / this.speed);
            this._updateTimer = setInterval(() => {
                if (!this._enabled) return;
                for (let i = 0; i < this.barCount; i++) {
                    const dist = Math.abs(i - this.barCount / 2) / (this.barCount / 2);
                    const maxH = 1.0 - dist * 0.35;
                    this._target[i] = (0.12 + 0.88 * Math.random() * Math.random()) * maxH;
                }
            }, intervalMs);
        }

        _barColor(ctx, i, yTop, yBottom, alphaScale = 1.0) {
            if (this.colorMode === 'rainbow') {
                const hue = (i * (360 / this.barCount) + this._phase * 22) % 360;
                const g = ctx.createLinearGradient(0, yTop, 0, yBottom);
                g.addColorStop(0, `hsla(${hue.toFixed(0)}, 90%, 72%, ${0.9 * alphaScale})`);
                g.addColorStop(1, `hsla(${((hue + 25) % 360).toFixed(0)}, 85%, 45%, ${alphaScale})`);
                return g;
            }
            if (this.colorMode === 'custom') {
                const g = ctx.createLinearGradient(0, yTop, 0, yBottom);
                g.addColorStop(0, this.colorTop);
                g.addColorStop(1, this.colorBottom);
                return g;
            }
            const h = this.hue;
            const g = ctx.createLinearGradient(0, yTop, 0, yBottom);
            g.addColorStop(0, `hsla(${h.toFixed(0)}, 85%, 72%, ${0.88 * alphaScale})`);
            g.addColorStop(0.55, `hsla(${h.toFixed(0)}, 80%, 55%, ${0.94 * alphaScale})`);
            g.addColorStop(1, `hsla(${h.toFixed(0)}, 75%, 38%, ${alphaScale})`);
            return g;
        }

        _draw() {
            if (!this._enabled) return;
            this._animFrame = requestAnimationFrame(() => this._draw());

            const canvas = this._activeCanvas();
            if (!canvas) return;
            this._syncCanvasSize();
            const ctx = canvas.getContext('2d');
            const W = canvas.width;
            const H = canvas.height;
            if (W <= 0 || H <= 0) return;

            ctx.clearRect(0, 0, W, H);
            this._phase += 0.03 * this.speed;

            const n = this.barCount;
            const lerp = Math.min(0.35, 0.14 * this.speed);
            for (let i = 0; i < n; i++) {
                this._current[i] += (this._target[i] - this._current[i]) * lerp;
            }

            if (this.style === 'wave') {
                this._drawWave(ctx, W, H, n);
                return;
            }

            const gap = this.gapPx;
            const barW = Math.max(2, (W - gap * (n - 1)) / n);

            for (let i = 0; i < n; i++) {
                const amp = Math.max(0.03, Math.min(1.0, this._current[i]));
                const x = i * (barW + gap);

                if (this.style === 'mirrored') {
                    const barH = Math.max(4, amp * (H - 4));
                    const y = (H - barH) * 0.5;
                    const r = Math.min(barW * 0.5, 4);
                    ctx.fillStyle = this._barColor(ctx, i, y, y + barH);
                    this._fillRoundedRect(ctx, x, y, barW, barH, r);
                } else if (this.style === 'dots') {
                    const centerY = H - Math.max(barW * 0.6, amp * (H - barW));
                    const dotR = Math.max(2.5, Math.min(barW * 0.48, 7));
                    ctx.fillStyle = this._barColor(ctx, i, centerY - dotR, H, 0.25);
                    ctx.fillRect(x + barW * 0.5 - 1, centerY, 2, H - centerY);
                    ctx.fillStyle = this._barColor(ctx, i, centerY - dotR, centerY + dotR);
                    ctx.beginPath();
                    ctx.arc(x + barW * 0.5, centerY, dotR, 0, Math.PI * 2);
                    ctx.fill();
                } else if (this.style === 'blocks') {
                    const barH = Math.max(4, amp * H);
                    const blockH = 5;
                    const step = blockH + 3;
                    const count = Math.max(1, Math.floor(barH / step));
                    const col = this._barColor(ctx, i, H - barH, H);
                    ctx.fillStyle = col;
                    for (let b = 0; b < count; b++) {
                        const by = H - (b + 1) * step + 2;
                        if (by < 0) break;
                        this._fillRoundedRect(ctx, x, by, barW, blockH, 2);
                    }
                } else {
                    const barH = Math.max(3, amp * H);
                    const y = H - barH;
                    const r = Math.min(barW * 0.5, 4);
                    ctx.fillStyle = this._barColor(ctx, i, y, H);
                    this._fillRoundedRect(ctx, x, y, barW, barH, r);
                }
            }
        }

        _drawWave(ctx, W, H, n) {
            const pts = [];
            for (let i = 0; i < n; i++) {
                const amp = Math.max(0.05, Math.min(1.0, this._current[i]));
                pts.push({
                    x: (n === 1) ? 0 : (i / (n - 1)) * W,
                    y: H - amp * (H * 0.82) - 4
                });
            }

            const buildCurve = () => {
                ctx.beginPath();
                ctx.moveTo(pts[0].x, pts[0].y);
                for (let i = 1; i < pts.length; i++) {
                    const xc = (pts[i - 1].x + pts[i].x) * 0.5;
                    const yc = (pts[i - 1].y + pts[i].y) * 0.5;
                    ctx.quadraticCurveTo(pts[i - 1].x, pts[i - 1].y, xc, yc);
                }
                ctx.lineTo(pts[pts.length - 1].x, pts[pts.length - 1].y);
            };

            buildCurve();
            ctx.lineTo(W, H);
            ctx.lineTo(0, H);
            ctx.closePath();
            ctx.fillStyle = this._barColor(ctx, Math.floor(n / 2), 0, H, 0.35);
            ctx.fill();

            buildCurve();
            ctx.lineWidth = 2.5;
            ctx.strokeStyle = this._barColor(ctx, 0, 0, H, 1.0);
            ctx.stroke();
        }

        _fillRoundedRect(ctx, x, y, w, h, r) {
            const rad = Math.min(r, w * 0.5, h * 0.5);
            ctx.beginPath();
            ctx.moveTo(x + rad, y);
            ctx.lineTo(x + w - rad, y);
            ctx.quadraticCurveTo(x + w, y, x + w, y + rad);
            ctx.lineTo(x + w, y + h - rad);
            ctx.quadraticCurveTo(x + w, y + h, x + w - rad, y + h);
            ctx.lineTo(x + rad, y + h);
            ctx.quadraticCurveTo(x, y + h, x, y + h - rad);
            ctx.lineTo(x, y + rad);
            ctx.quadraticCurveTo(x, y, x + rad, y);
            ctx.closePath();
            ctx.fill();
        }
    }

    class NowPlayingWidget {
        constructor() {
            const params = new URLSearchParams(location.search);
            this.isPreviewPage =
                document.body.dataset.osmvPreview === '1' ||
                params.get('preview') === '1';
            this.presetParam = params.get('preset') || '';
            this.previewIdle = false;
            this.updateIntervalMs = 1000;
            this.autoReloadMs = 10 * 60 * 1000;

            this.config = Object.assign({}, DEFAULT_CONFIG, window.__OSMV_WIDGET_CONFIG || {});
            this.currentSong = null;
            this.lastRenderSig = '';
            this.lastFontUrl = '';
            this.currentHue = 260;

            this._hideTimer = null;
            this._widgetVisible = true;
            this._polling = false;
            this._colorCanvas = document.getElementById('color-canvas');
            this._colorCtx = this._colorCanvas.getContext('2d');
            this._visualizer = new AudioVisualizer();

            this.elements = {
                stack: document.getElementById('obs-stack'),
                container: document.getElementById('now-playing-widget'),
                content: document.getElementById('content'),
                artWrap: document.getElementById('album-art-container'),
                albumArt: document.getElementById('album-art'),
                albumPlaceholder: document.getElementById('album-placeholder'),
                badge: document.getElementById('track-badge'),
                title: document.getElementById('title'),
                artist: document.getElementById('artist'),
                album: document.getElementById('album'),
                studioStyle: document.getElementById('osmv-studio-css'),
                customStyle: document.getElementById('osmv-custom-css'),
                gfontLink: document.getElementById('osmv-gfont')
            };

            this.applyConfig(this.config);
            this.startPolling();
        }

        toggleDemoState() {
            this.previewIdle = !this.previewIdle;
            this.poll();
        }

        async fetchJson(url) {
            const sep = url.includes('?') ? '&' : '?';
            const resp = await fetch(`${url}${sep}t=${Date.now()}`, { cache: 'no-store' });
            if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
            return await resp.json();
        }

        loadScriptGlobal(scriptId, fileName, globalVar) {
            return new Promise((resolve) => {
                const prev = document.getElementById(scriptId);
                if (prev) prev.remove();
                try {
                    delete window[globalVar];
                } catch (_) {
                    window[globalVar] = undefined;
                }
                const sc = document.createElement('script');
                sc.id = scriptId;
                sc.src = `${fileName}?t=${Date.now()}`;
                sc.onload = () => {
                    const hasVal = Object.prototype.hasOwnProperty.call(window, globalVar) &&
                        window[globalVar] !== undefined;
                    resolve({ ok: hasVal, value: hasVal ? window[globalVar] : null });
                };
                sc.onerror = () => resolve({ ok: false, value: null });
                document.body.appendChild(sc);
            });
        }

        async fetchJsonOrScriptFallback(jsonFile, scriptId, jsFile, globalVar) {
            // Avoid browser fetch() on file:// URLs since Chromium blocks local fetch with CORS errors
            if (location.protocol !== 'file:') {
                try {
                    return { ok: true, value: await this.fetchJson(jsonFile) };
                } catch (_) {
                }
            }
            return await this.loadScriptGlobal(scriptId, jsFile, globalVar);
        }

        async fetchConfig() {
            const res = await this.fetchJsonOrScriptFallback(
                'widget_config.json', 'osmv-cfg-loader', 'widget_config.js', '__OSMV_WIDGET_CONFIG');
            return res.ok ? res.value : null;
        }

        async fetchCurrentSong() {
            const res = await this.fetchJsonOrScriptFallback(
                'current_song.json', 'osmv-song-loader', 'current_song.js', '__OSMV_CURRENT_SONG');
            if (res.ok) {
                return res.value;
            }
            if (this.isPreviewPage) {
                return this.previewIdle ? null : DEMO_SONG;
            }
            return null;
        }

        applyConfig(raw) {
            if (!raw) return;
            let resolved = raw;
            if (this.presetParam && raw.presets && typeof raw.presets === 'object') {
                const target = this.presetParam.toLowerCase();
                const stripped = target.startsWith('user:') ? target.slice(5) : target;
                const keys = Object.keys(raw.presets);
                const matchKey = keys.find((k) => k.toLowerCase() === target) ||
                    keys.find((k) => k.toLowerCase() === stripped);
                if (matchKey) resolved = raw.presets[matchKey];
            }
            const cfg = Object.assign({}, DEFAULT_CONFIG, resolved);
            this.config = cfg;

            // Studio CSS injection (unless user enabled raw custom override)
            if (cfg.customCssEnabled && cfg.customCssRawOverride) {
                this.elements.studioStyle.textContent = '';
            } else {
                this.elements.studioStyle.textContent = cfg.generatedStudioCss || '';
            }

            // User Custom CSS
            this.elements.customStyle.textContent = cfg.customCssEnabled ? (cfg.customCss || '') : '';

            // Optional Google Fonts loading
            if (cfg.loadGoogleFont && cfg.fontFamily && !SYSTEM_FONTS.has(cfg.fontFamily.toLowerCase())) {
                const fam = encodeURIComponent(cfg.fontFamily.trim()).replace(/%20/g, '+');
                const url = `https://fonts.googleapis.com/css2?family=${fam}:wght@400;600;700;800&display=swap`;
                if (url !== this.lastFontUrl) {
                    this.lastFontUrl = url;
                    this.elements.gfontLink.href = url;
                }
            } else if (this.lastFontUrl) {
                this.lastFontUrl = '';
                this.elements.gfontLink.href = '';
            }

            // Layout & transition classes
            const c = this.elements.container;
            const layoutCls = `layout-${cfg.layout || 'horizontal'}`;
            const transCls = `transition-${cfg.transitionStyle || 'fade'}`;
            const hidden = c.classList.contains('widget-hidden');
            c.className = `widget-container ${layoutCls} ${transCls}` + (hidden ? ' widget-hidden' : '');

            // Marquee toggle classes
            [this.elements.title, this.elements.artist, this.elements.album].forEach(el => {
                el.classList.toggle('no-marquee', !cfg.marqueeScroll);
            });

            // Artwork visibility and vinyl mode
            const artSize = Math.max(24, Number(cfg.artSizePx) || 80);
            const isRound = Number(cfg.artRadiusPx) >= artSize * 0.45;
            this.elements.artWrap.style.display = cfg.showAlbumArt ? 'flex' : 'none';
            this.elements.artWrap.classList.toggle('is-vinyl', Boolean(cfg.spinArtwork && isRound));
            this.elements.albumArt.classList.toggle('is-spinning', Boolean(cfg.spinArtwork));

            this._visualizer.configure(cfg);
            if (!cfg.dynamicColor) {
                const fallbackHue = parseCssColorToHue(cfg.accentColorCss);
                this.currentHue = fallbackHue;
                this._visualizer.setHue(fallbackHue);
                this.resetDynamicColor();
            }
        }

        extractDominantColor(imgSrc) {
            return new Promise((resolve) => {
                const img = new Image();
                img.crossOrigin = 'anonymous';
                img.onload = () => {
                    try {
                        const SIZE = 80;
                        this._colorCanvas.width = SIZE;
                        this._colorCanvas.height = SIZE;
                        this._colorCtx.drawImage(img, 0, 0, SIZE, SIZE);
                        const data = this._colorCtx.getImageData(0, 0, SIZE, SIZE).data;

                        const buckets = new Array(36).fill(0).map(() => ({ r: 0, g: 0, b: 0, count: 0 }));
                        for (let i = 0; i < data.length; i += 4) {
                            const r = data[i], g = data[i + 1], b = data[i + 2], a = data[i + 3];
                            if (a < 128) continue;
                            const { h, s, l } = this.rgbToHsl(r, g, b);
                            if (s < 0.15 || l < 0.1 || l > 0.9) continue;
                            const bucket = Math.min(35, Math.floor(h / 10));
                            buckets[bucket].r += r;
                            buckets[bucket].g += g;
                            buckets[bucket].b += b;
                            buckets[bucket].count++;
                        }
                        const best = buckets.reduce((a, b) => (b.count > a.count ? b : a), buckets[0]);
                        if (best.count === 0) { resolve(null); return; }
                        resolve({
                            r: Math.round(best.r / best.count),
                            g: Math.round(best.g / best.count),
                            b: Math.round(best.b / best.count)
                        });
                    } catch (_) {
                        resolve(null);
                    }
                };
                img.onerror = () => resolve(null);
                img.src = imgSrc;
            });
        }

        rgbToHsl(r, g, b) {
            r /= 255; g /= 255; b /= 255;
            const max = Math.max(r, g, b), min = Math.min(r, g, b);
            let h = 0, s = 0, l = (max + min) / 2;
            if (max !== min) {
                const d = max - min;
                s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
                if (max === r) h = ((g - b) / d + (g < b ? 6 : 0)) / 6;
                else if (max === g) h = ((b - r) / d + 2) / 6;
                else h = ((r - g) / d + 4) / 6;
            }
            return { h: h * 360, s, l };
        }

        applyDynamicColor(color) {
            const c = this.elements.container;
            if (!color) { this.resetDynamicColor(); return; }
            const { r, g, b } = color;
            const { h } = this.rgbToHsl(r, g, b);
            this.currentHue = h;
            this._visualizer.setHue(h);

            c.style.setProperty('--osmv-album-hue', h.toFixed(0));
            c.style.setProperty('--osmv-album-rgb', `${r}, ${g}, ${b}`);

            if (this.config.customCssEnabled && this.config.customCssRawOverride) {
                return;
            }

            const mode = this.config.dynamicColorMode || 'card';
            const hue = h.toFixed(0);
            if (mode === 'accent') {
                c.style.background = '';
                c.style.borderColor = `hsl(${hue}, 82%, 58%)`;
                c.style.boxShadow = `0 10px 32px rgba(0, 0, 0, 0.5), 0 0 24px hsla(${hue}, 85%, 52%, 0.45)`;
                this.elements.badge.style.background = `hsl(${hue}, 78%, 48%)`;
            } else if (mode === 'subtle') {
                c.style.background = `linear-gradient(135deg, hsla(${hue}, 65%, 22%, 0.88), rgba(14, 16, 24, 0.90))`;
                c.style.borderColor = `hsla(${hue}, 75%, 55%, 0.55)`;
                c.style.boxShadow = `0 10px 32px rgba(0, 0, 0, 0.48), 0 0 20px hsla(${hue}, 80%, 40%, 0.30)`;
                this.elements.badge.style.background = `hsl(${hue}, 72%, 45%)`;
            } else {
                c.style.background = `hsl(${hue}, 72%, 20%)`;
                c.style.borderColor = `hsl(${hue}, 72%, 48%)`;
                c.style.boxShadow =
                    `0 8px 32px rgba(0, 0, 0, 0.5), ` +
                    `0 0 0 1px hsl(${hue}, 80%, 55%) inset, ` +
                    `0 0 28px hsla(${hue}, 80%, 38%, 0.45)`;
                this.elements.badge.style.background = `hsl(${hue}, 80%, 50%)`;
            }
        }

        resetDynamicColor() {
            const c = this.elements.container;
            c.style.background = '';
            c.style.borderColor = '';
            c.style.boxShadow = '';
            this.elements.badge.style.background = '';
            const fallbackHue = parseCssColorToHue(this.config.accentColorCss);
            this.currentHue = fallbackHue;
            this._visualizer.setHue(fallbackHue);
        }

        scheduleHide() {
            if (!this.config.hideWhenIdle) {
                this.cancelHide();
                return;
            }
            if (this._hideTimer) return;
            const delayMs = Math.max(0, Number(this.config.hideDelaySec) || 0) * 1000;
            this._hideTimer = setTimeout(() => this.hideWidget(), delayMs);
        }

        cancelHide() {
            if (this._hideTimer) {
                clearTimeout(this._hideTimer);
                this._hideTimer = null;
            }
            if (!this._widgetVisible) this.showWidget();
        }

        hideWidget() {
            this._widgetVisible = false;
            this._hideTimer = null;
            this.elements.container.classList.add('widget-hidden');
        }

        showWidget() {
            this._widgetVisible = true;
            this.elements.container.classList.remove('widget-hidden');
        }

        async updateDisplay(songData) {
            const cfg = this.config;
            const isPreview = this.isPreviewPage && !this.previewIdle;
            if (!songData && isPreview) {
                songData = DEMO_SONG;
            }

            const isInactive = !songData ||
                songData.status === 'paused' ||
                songData.status === 'closed' ||
                songData.status === 'stopped';

            if (isInactive && !isPreview) {
                this.scheduleHide();
            } else {
                this.cancelHide();
            }

            // Visualizer state
            const vizWanted = this.presetParam
                ? Boolean(cfg.audioVisualizer)
                : Boolean(cfg.audioVisualizer || (songData && songData.audioVisualizer === true));
            if (vizWanted && (!isInactive || isPreview)) {
                this._visualizer.enable();
            } else {
                this._visualizer.disable();
            }

            // Pause or play vinyl spin
            this.elements.albumArt.classList.toggle('spin-paused', isInactive && !isPreview);

            const artSrc = songData
                ? (songData.thumbnailDataUri || (songData.thumbnail ? `data:image/jpeg;base64,${songData.thumbnail}` : ''))
                : '';

            const renderSig = [
                songData ? songData.title : 'NULL',
                songData ? songData.artist : '',
                songData ? songData.album : '',
                songData ? songData.player : '',
                songData ? songData.status : '',
                artSrc.length,
                cfg.showAlbumArt,
                cfg.showTitle,
                cfg.showArtist,
                cfg.showAlbum,
                cfg.showStatusBadge,
                cfg.titleFormat,
                cfg.artistFormat,
                cfg.albumFormat,
                cfg.badgeFormat,
                cfg.idleText,
                cfg.marqueeScroll,
                cfg.scrollSpeed,
                cfg.dynamicColor,
                cfg.dynamicColorMode,
                cfg.customCssRawOverride
            ].join('|');

            if (renderSig === this.lastRenderSig) return;
            const trackChanged = !this.currentSong || !songData ||
                this.currentSong.title !== songData.title ||
                this.currentSong.artist !== songData.artist;
            this.lastRenderSig = renderSig;

            if (trackChanged && this.currentSong) {
                await this.fadeOut();
            }

            if (!songData) {
                this.elements.badge.style.display = 'none';
                this.elements.title.style.display = 'block';
                this.elements.artist.style.display = 'none';
                this.elements.album.style.display = 'none';
                this.setScrollText(this.elements.title, cfg.idleText || 'Nothing currently playing');
                this.elements.albumArt.style.display = 'none';
                this.elements.albumPlaceholder.style.display = cfg.showAlbumArt ? 'flex' : 'none';
                this.resetDynamicColor();
                await this.fadeIn();
                this.currentSong = null;
                return;
            }

            // Visibility & formatted content
            const badgeText = cfg.showStatusBadge ? formatTokens(cfg.badgeFormat, songData).trim() : '';
            this.elements.badge.style.display = badgeText ? 'inline-block' : 'none';
            if (badgeText) this.elements.badge.textContent = badgeText;

            this.elements.title.style.display = cfg.showTitle ? 'block' : 'none';
            if (cfg.showTitle) {
                this.setScrollText(this.elements.title, formatTokens(cfg.titleFormat, songData));
            }

            const artistText = cfg.showArtist ? formatTokens(cfg.artistFormat, songData) : '';
            this.elements.artist.style.display = artistText ? 'block' : 'none';
            if (artistText) {
                this.setScrollText(this.elements.artist, artistText);
            }

            const albumText = (cfg.showAlbum && songData.album) ? formatTokens(cfg.albumFormat, songData) : '';
            this.elements.album.style.display = albumText ? 'block' : 'none';
            if (albumText) {
                this.setScrollText(this.elements.album, albumText);
            }

            // Album artwork & dynamic color
            if (cfg.showAlbumArt && artSrc) {
                this.elements.albumArt.src = artSrc;
                this.elements.albumArt.style.display = 'block';
                this.elements.albumPlaceholder.style.display = 'none';
                if (cfg.dynamicColor) {
                    const color = await this.extractDominantColor(artSrc);
                    this.applyDynamicColor(color);
                } else {
                    this.resetDynamicColor();
                }
            } else {
                this.elements.albumArt.style.display = 'none';
                this.elements.albumPlaceholder.style.display = cfg.showAlbumArt ? 'flex' : 'none';
                this.resetDynamicColor();
            }

            await this.fadeIn();
            this.currentSong = songData;
        }

        setScrollText(container, text) {
            const span = container.querySelector('.scroll-text');
            if (!span) return;
            if (span._scrollTimer) {
                clearTimeout(span._scrollTimer);
                span._scrollTimer = null;
            }
            span.style.transition = 'none';
            span.style.transform = 'translateX(0)';
            span.textContent = text;
            if (!this.config.marqueeScroll) return;

            requestAnimationFrame(() => requestAnimationFrame(() => {
                const overflow = span.scrollWidth - container.clientWidth;
                if (overflow > 2) this.startScrollCycle(span, overflow);
            }));
        }

        startScrollCycle(span, overflow) {
            const spd = Math.max(0.4, Math.min(2.5, Number(this.config.scrollSpeed) || 1.0));
            const baseMs = Math.min(18000, Math.max(5000, (overflow / 50) * 1000));
            const duration = Math.round(baseMs / spd);
            const pause = 2800;
            const doCycle = () => {
                span._scrollTimer = setTimeout(() => {
                    span.style.transition = `transform ${duration}ms linear`;
                    span.style.transform = `translateX(-${overflow}px)`;
                    span._scrollTimer = setTimeout(() => {
                        span._scrollTimer = setTimeout(() => {
                            span.style.transition = `transform ${duration}ms linear`;
                            span.style.transform = 'translateX(0)';
                            span._scrollTimer = setTimeout(doCycle, duration);
                        }, pause);
                    }, duration);
                }, pause);
            };
            doCycle();
        }

        fadeOut() {
            return new Promise((resolve) => {
                this.elements.content.classList.remove('fade-in');
                this.elements.content.classList.add('fade-out');
                setTimeout(resolve, 240);
            });
        }

        fadeIn() {
            return new Promise((resolve) => {
                this.elements.content.classList.remove('fade-out');
                this.elements.content.classList.add('fade-in');
                setTimeout(resolve, 240);
            });
        }

        async poll() {
            if (this._polling) return;
            this._polling = true;
            try {
                const [cfgData, songData] = await Promise.all([
                    this.fetchConfig(),
                    this.fetchCurrentSong()
                ]);
                if (cfgData) this.applyConfig(cfgData);
                await this.updateDisplay(songData);
            } finally {
                this._polling = false;
            }
        }

        startPolling() {
            this.poll();
            setInterval(() => this.poll(), this.updateIntervalMs);
            if (!this.isPreviewPage) {
                setTimeout(() => location.reload(), this.autoReloadMs);
            }
        }
    }

    document.addEventListener('DOMContentLoaded', () => {
        window.__OSMV_WIDGET = new NowPlayingWidget();
    });
})();
