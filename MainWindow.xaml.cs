using System;
using System.Drawing;
using System.IO;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Forms;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using Windows.Media.Control;
using Windows.Storage.Streams;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;

namespace OBS_StreamMusicViewer
{
    public partial class MainWindow : Window
    {
        private DispatcherTimer _timer;
        private string _outputFilePath;
        private string _settingsFilePath;
        private GlobalSystemMediaTransportControlsSessionManager _sessionManager;

        // System tray icon
        private NotifyIcon _notifyIcon;
        private bool _isClosing = false; // true only when "Quitter" is clicked

        // Option : couleur dynamique
        private bool _dynamicColorEnabled = false;

        // Option : visualiseur audio
        private bool _audioVisualizerEnabled = false;

        // Option : Discord Rich Presence
        private bool _discordRpcEnabled = false;
        private string _discordClientId = "1479531788731809913"; // ID personnalisé
        private DiscordRpcService _discordRpc;
        
        // iTunes API Helpers pour Discord RPC
        private static readonly System.Net.Http.HttpClient _httpClient = new System.Net.Http.HttpClient();
        private string _lastDiscordTitle = null;
        private string _lastDiscordArtist = null;
        private string _currentCoverUrl = null;
        private bool _isFetchingCover = false;

        public MainWindow()
        {
            InitializeComponent();
            _outputFilePath  = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "current_song.json");
            _settingsFilePath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "settings.json");

            _timer = new DispatcherTimer();
            _timer.Interval = TimeSpan.FromSeconds(1);
            _timer.Tick += Timer_Tick;

            LoadSettings();
            InitializeTrayIcon();
            InitializeMediaManager();
        }

        // ─── Settings ────────────────────────────────────────────────────────────

        private void LoadSettings()
        {
            try
            {
                if (File.Exists(_settingsFilePath))
                {
                    string json = File.ReadAllText(_settingsFilePath);
                    using var doc = JsonDocument.Parse(json);
                    if (doc.RootElement.TryGetProperty("dynamicColor", out var val))
                        _dynamicColorEnabled = val.GetBoolean();
                    if (doc.RootElement.TryGetProperty("audioVisualizer", out var viz))
                        _audioVisualizerEnabled = viz.GetBoolean();
                    if (doc.RootElement.TryGetProperty("discordRpc", out var rpc))
                        _discordRpcEnabled = rpc.GetBoolean();
                    if (doc.RootElement.TryGetProperty("discordClientId", out var cid))
                        _discordClientId = cid.GetString();
                }
            }
            catch { /* ignore */ }

            DynamicColorCheckBox.IsChecked = _dynamicColorEnabled;
            AudioVisualizerCheckBox.IsChecked = _audioVisualizerEnabled;
            DiscordRpcCheckBox.IsChecked = _discordRpcEnabled;
            DiscordSettingsPanel.Visibility = _discordRpcEnabled ? Visibility.Visible : Visibility.Collapsed;
            DiscordClientIdTextBox.Text = _discordClientId;

            // Auto-démarrage Discord RPC
            if (_discordRpcEnabled && !string.IsNullOrWhiteSpace(_discordClientId))
            {
                _discordRpc = new DiscordRpcService();
                _discordRpc.Initialize(_discordClientId);
            }
        }

        private void SaveSettings()
        {
            try
            {
                var data = new { 
                    dynamicColor = _dynamicColorEnabled, 
                    audioVisualizer = _audioVisualizerEnabled,
                    discordRpc = _discordRpcEnabled,
                    discordClientId = _discordClientId
                };
                var options = new JsonSerializerOptions { WriteIndented = true };
                File.WriteAllText(_settingsFilePath, JsonSerializer.Serialize(data, options));
            }
            catch { /* ignore */ }
        }

        private void DynamicColorCheckBox_Changed(object sender, RoutedEventArgs e)
        {
            _dynamicColorEnabled = DynamicColorCheckBox.IsChecked == true;
            SaveSettings();
        }

        private void AudioVisualizerCheckBox_Changed(object sender, RoutedEventArgs e)
        {
            _audioVisualizerEnabled = AudioVisualizerCheckBox.IsChecked == true;
            SaveSettings();
        }

        private void DiscordRpcCheckBox_Changed(object sender, RoutedEventArgs e)
        {
            _discordRpcEnabled = DiscordRpcCheckBox.IsChecked == true;
            DiscordSettingsPanel.Visibility = _discordRpcEnabled ? Visibility.Visible : Visibility.Collapsed;
            SaveSettings();

            if (_discordRpcEnabled && !string.IsNullOrWhiteSpace(_discordClientId))
            {
                _discordRpc ??= new DiscordRpcService();
                _discordRpc.Initialize(_discordClientId);
            }
            else
            {
                _discordRpc?.ClearPresence();
                _discordRpc?.Dispose();
                _discordRpc = null;
            }
        }

        private void DiscordClientIdTextBox_LostFocus(object sender, RoutedEventArgs e)
        {
            if (_discordClientId != DiscordClientIdTextBox.Text)
            {
                _discordClientId = DiscordClientIdTextBox.Text;
                SaveSettings();

                if (_discordRpcEnabled)
                {
                    _discordRpc?.ClearPresence();
                    _discordRpc?.Dispose();
                    
                    if (!string.IsNullOrWhiteSpace(_discordClientId))
                    {
                        _discordRpc = new DiscordRpcService();
                        _discordRpc.Initialize(_discordClientId);
                    }
                }
            }
        }

        // ─── System Tray ────────────────────────────────────────────────────────

        private void InitializeTrayIcon()
        {
            _notifyIcon = new NotifyIcon();

            // Charger l'icône depuis le fichier .ico embarqué
            string icoPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "OSMV_logo.ico");
            if (File.Exists(icoPath))
                _notifyIcon.Icon = new Icon(icoPath);
            else
                _notifyIcon.Icon = SystemIcons.Application;

            _notifyIcon.Text = "OBS Stream Music Viewer";
            _notifyIcon.Visible = true;

            // Double-clic → restaurer la fenêtre
            _notifyIcon.DoubleClick += (s, e) => RestoreWindow();

            // Menu contextuel
            var contextMenu = new ContextMenuStrip();

            var menuShow = new ToolStripMenuItem("Afficher");
            menuShow.Font = new Font(menuShow.Font, System.Drawing.FontStyle.Bold);
            menuShow.Click += (s, e) => RestoreWindow();

            var menuQuit = new ToolStripMenuItem("Quitter");
            menuQuit.Click += (s, e) =>
            {
                _isClosing = true;
                System.Windows.Application.Current.Shutdown();
            };

            contextMenu.Items.Add(menuShow);
            contextMenu.Items.Add(new ToolStripSeparator());
            contextMenu.Items.Add(menuQuit);

            _notifyIcon.ContextMenuStrip = contextMenu;
        }

        private void RestoreWindow()
        {
            Show();
            WindowState = WindowState.Normal;
            ShowInTaskbar = true;
            Activate();
        }

        // Quand la fenêtre est minimisée → la cacher dans le tray
        private void Window_StateChanged(object sender, EventArgs e)
        {
            if (WindowState == WindowState.Minimized)
            {
                Hide();
                ShowInTaskbar = false;
                _notifyIcon.ShowBalloonTip(
                    2000,
                    "OBS Stream Music Viewer",
                    "L'application tourne en arrière-plan.",
                    ToolTipIcon.Info);
            }
        }

        // Interception de la fermeture : minimiser au lieu de quitter (sauf si "Quitter" cliqué)
        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (!_isClosing)
            {
                e.Cancel = true;
                WindowState = WindowState.Minimized;
                return;
            }

            // Fermeture réelle
            _timer?.Stop();
            _discordRpc?.Dispose();
            _notifyIcon?.Dispose();
            try { WriteJsonDump(null, null, null, "closed", null); } catch { }
        }

        // ─── Media Manager ───────────────────────────────────────────────────────

        private async void InitializeMediaManager()
        {
            try
            {
                _sessionManager = await GlobalSystemMediaTransportControlsSessionManager.RequestAsync();
                _timer.Start();
                UpdateUI("Manager initialized, awaiting track...", "---", "", null);
            }
            catch (Exception ex)
            {
                ErrorText.Text = "Failed to access Windows Media API: " + ex.Message;
            }
        }

        private async void Timer_Tick(object sender, EventArgs e)
        {
            try
            {
                if (_sessionManager == null) return;

                var session = _sessionManager.GetCurrentSession();

                if (session == null)
                {
                    WriteJsonDump(null, null, null, "closed", null);
                    UpdateUI("No active media", "---", "closed", null);
                    return;
                }

                var mediaProps = await session.TryGetMediaPropertiesAsync();
                if (mediaProps == null)
                {
                    WriteJsonDump(null, null, null, "closed", null);
                    UpdateUI("No media properties", "---", "closed", null);
                    return;
                }

                string title  = mediaProps.Title       ?? "Unknown Title";
                string artist = mediaProps.Artist      ?? "Unknown Artist";
                string album  = mediaProps.AlbumTitle  ?? "";

                var playbackInfo = session.GetPlaybackInfo();
                string status = "unknown";
                bool isPlaying = false;
                if (playbackInfo != null)
                {
                    switch (playbackInfo.PlaybackStatus)
                    {
                        case GlobalSystemMediaTransportControlsSessionPlaybackStatus.Closed:   status = "closed";   break;
                        case GlobalSystemMediaTransportControlsSessionPlaybackStatus.Opened:   status = "opened";   break;
                        case GlobalSystemMediaTransportControlsSessionPlaybackStatus.Changing: status = "changing"; break;
                        case GlobalSystemMediaTransportControlsSessionPlaybackStatus.Stopped:  status = "stopped";  break;
                        case GlobalSystemMediaTransportControlsSessionPlaybackStatus.Playing:  
                            status = "playing"; 
                            isPlaying = true;
                            break;
                        case GlobalSystemMediaTransportControlsSessionPlaybackStatus.Paused:   status = "paused";   break;
                    }
                }

                string base64Image = null;
                BitmapImage bitmapImage = null;

                if (mediaProps.Thumbnail != null)
                {
                    try
                    {
                        using (var stream = await mediaProps.Thumbnail.OpenReadAsync())
                        {
                            byte[] buffer = new byte[stream.Size];
                            await stream.ReadAsync(buffer.AsBuffer(), (uint)stream.Size, InputStreamOptions.None);
                            base64Image = Convert.ToBase64String(buffer);

                            bitmapImage = new BitmapImage();
                            bitmapImage.BeginInit();
                            bitmapImage.StreamSource = new MemoryStream(buffer);
                            bitmapImage.CacheOption = BitmapCacheOption.OnLoad;
                            bitmapImage.EndInit();
                        }
                    }
                    catch (Exception) { /* Handle thumbnail error gracefully */ }
                }

                UpdateUI(title, artist, status, bitmapImage);
                WriteJsonDump(title, artist, album, status, base64Image);
                UpdateDiscordRpc(title, artist, isPlaying);
            }
            catch (Exception ex)
            {
                ErrorText.Text = "Error during tick: " + ex.Message;
            }
        }

        private async void UpdateDiscordRpc(string title, string artist, bool isPlaying)
        {
            if (!_discordRpcEnabled || _discordRpc == null) return;

            if (title != _lastDiscordTitle || artist != _lastDiscordArtist)
            {
                _lastDiscordTitle = title;
                _lastDiscordArtist = artist;
                _currentCoverUrl = null;

                if (!_isFetchingCover && !string.IsNullOrEmpty(title))
                {
                    _isFetchingCover = true;
                    try
                    {
                        string query = Uri.EscapeDataString($"{artist} {title}");
                        string url = $"https://itunes.apple.com/search?term={query}&entity=song&limit=1";
                        using var request = new System.Net.Http.HttpRequestMessage(System.Net.Http.HttpMethod.Get, url);
                        var response = await _httpClient.SendAsync(request);
                        if (response.IsSuccessStatusCode)
                        {
                            var json = await response.Content.ReadAsStringAsync();
                            using var doc = JsonDocument.Parse(json);
                            var results = doc.RootElement.GetProperty("results");
                            if (results.GetArrayLength() > 0)
                            {
                                var artwork = results[0].GetProperty("artworkUrl100").GetString();
                                _currentCoverUrl = artwork?.Replace("100x100bb", "512x512bb");
                            }
                        }
                    }
                    catch { }
                    finally { _isFetchingCover = false; }
                }
            }
            
            _discordRpc.UpdatePresence(title, artist, isPlaying, _currentCoverUrl);
        }

        private void UpdateUI(string title, string artist, string status, BitmapImage image)
        {
            TitleText.Text  = title;
            ArtistText.Text = artist;
            StatusText.Text = "Status: " + status;

            if (image != null)
                AlbumArtBrush.ImageSource = image;
            else
                AlbumArtBrush.ImageSource = null;
        }

        private void WriteJsonDump(string title, string artist, string album, string status, string thumbnailB64)
        {
            try
            {
                object data;

                if (string.IsNullOrEmpty(title) && string.IsNullOrEmpty(artist))
                {
                    data = null; // No song playing
                }
                else
                {
                    data = new
                    {
                        title        = title,
                        artist       = artist,
                        album        = album,
                        thumbnail    = thumbnailB64,
                        status          = status,
                        dynamicColor    = _dynamicColorEnabled,
                        audioVisualizer = _audioVisualizerEnabled,
                        timestamp       = DateTime.Now.ToString("o")
                    };
                }

                var options = new JsonSerializerOptions { WriteIndented = true };
                string jsonString = JsonSerializer.Serialize(data, options);
                File.WriteAllText(_outputFilePath, jsonString);
            }
            catch (Exception ex)
            {
                ErrorText.Text = "JSON save error: " + ex.Message;
            }
        }
    }
}
