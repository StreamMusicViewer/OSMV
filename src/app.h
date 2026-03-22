#pragma once
#include <QObject>
#include <QTimer>
#include <memory>

class IMediaProvider;
class MainWindow;

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

private slots:
  void tick();
  void onSettingsChanged();

private:
  void loadSettings();
  void saveSettings();

  // Components
  std::unique_ptr<IMediaProvider> m_provider;
  std::unique_ptr<MainWindow> m_window;
  QTimer m_timer;

  // Settings
  bool m_dynamicColor = false;
  bool m_audioVisualizer = false;

  // State
  QString m_lastSongKey; // "title|artist|artUrl" for thumb caching
  QString m_cachedThumbnailB64;
};
