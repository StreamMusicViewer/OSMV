#pragma once
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <memory>

class IMediaProvider;
class MainWindow;
class DiscordRpc;

/// Main application orchestrator.
/// Creates the provider, starts the 1-second poll timer, writes JSON,
/// manages Discord RPC, and owns the settings.
class App : public QObject {
  Q_OBJECT
public:
  explicit App(QObject *parent = nullptr);
  ~App();

  void start();

  // Settings (persisted to settings.json)
  bool dynamicColor() const { return m_dynamicColor; }
  bool audioVisualizer() const { return m_audioVisualizer; }
  bool discordRpc() const { return m_discordRpcEnabled; }
  QString discordClientId() const { return m_discordClientId; }

private slots:
  void tick();
  void onSettingsChanged();
  void onDiscordRpcToggled(bool enabled);
  void onDiscordClientIdChanged(const QString &id);

private:
  void loadSettings();
  void saveSettings();
  void syncDiscordRpc();
  void reinitDiscordRpc();
  void fetchItunesCover(const QString &title, const QString &artist);
  void updateDiscord(const QString &title, const QString &artist,
                     bool isPlaying);

  // Components
  std::unique_ptr<IMediaProvider> m_provider;
  std::unique_ptr<MainWindow> m_window;
  std::unique_ptr<DiscordRpc> m_discord;
  std::unique_ptr<QNetworkAccessManager> m_http;
  QTimer m_timer;

  // Settings
  bool m_dynamicColor = false;
  bool m_audioVisualizer = false;
  bool m_discordRpcEnabled = false;
  QString m_discordClientId = "1479531788731809913";

  // State
  QString m_lastSongKey; // "title|artist|artUrl" for thumb caching
  QString m_cachedThumbnailB64;
  QString m_lastDiscordTitle;
  QString m_lastDiscordArtist;
  QString m_currentCoverUrl;
  bool m_fetchingCover = false;
};
