using System;
using DiscordRPC;
using DiscordRPC.Logging;

using System.Threading;
using System.Threading.Tasks;

namespace OBS_StreamMusicViewer
{
    public class DiscordRpcService : IDisposable
    {
        private DiscordRpcClient _client;
        private bool _isInitialized = false;
        private CancellationTokenSource _clearCts;

        public void Initialize(string clientId)
        {
            if (_isInitialized) return;
            if (string.IsNullOrWhiteSpace(clientId)) return;

            try
            {
                _client = new DiscordRpcClient(clientId);
                _client.Logger = new ConsoleLogger() { Level = LogLevel.Warning };

                _client.OnReady += (sender, e) =>
                {
                    System.Diagnostics.Debug.WriteLine($"[Discord RPC] Initialisé pour l'utilisateur {e.User.Username}");
                };

                _client.Initialize();
                _isInitialized = true;
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"[Discord RPC] Erreur d'initialisation: {ex.Message}");
            }
        }

        public void UpdatePresence(string title, string artist, bool isPlaying, string coverUrl = null)
        {
            if (!_isInitialized || _client == null) return;

            try
            {
                if (string.IsNullOrEmpty(title) && string.IsNullOrEmpty(artist))
                {
                    ScheduleClearPresence();
                    return;
                }

                if (!isPlaying)
                {
                    ScheduleClearPresence();
                    return; // On ne met plus à jour la présence (elle va être effacée)
                }
                else
                {
                    CancelClearPresence();
                }

                _client.SetPresence(new RichPresence()
                {
                    Details = title,
                    State = $"de {artist}",
                    Assets = new Assets()
                    {
                        LargeImageKey = !string.IsNullOrWhiteSpace(coverUrl) ? coverUrl : "placeholder",
                        LargeImageText = $"{title} - {artist}",
                        SmallImageKey = "osmv_logo",
                        SmallImageText = "OBS Stream Music Viewer"
                    },
                    Buttons = new[]
                    {
                        new Button() { Label = "Site Web", Url = "https://streammusicviewer.github.io/site/" }
                    }
                });
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"[Discord RPC] Erreur UpdatePresence: {ex.Message}");
            }
        }

        public void ClearPresence()
        {
            if (!_isInitialized || _client == null) return;
            try
            {
                _client.ClearPresence();
            }
            catch { }
        }

        private async void ScheduleClearPresence()
        {
            if (_clearCts != null) return; // Déjà planifié
            _clearCts = new CancellationTokenSource();
            try
            {
                await Task.Delay(5000, _clearCts.Token);
                ClearPresence();
            }
            catch (TaskCanceledException) { }
            finally
            {
                _clearCts?.Dispose();
                _clearCts = null;
            }
        }

        private void CancelClearPresence()
        {
            if (_clearCts != null)
            {
                _clearCts.Cancel();
            }
        }

        public void Dispose()
        {
            if (_client != null)
            {
                CancelClearPresence();
                ClearPresence();
                _client.Dispose();
                _client = null;
            }
            _isInitialized = false;
        }
    }
}
