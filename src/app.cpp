#include "app.h"
#include "mainwindow.h"
#include "mediaprovider.h"
#include "utils.h"

#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

// ── Constructor
// ───────────────────────────────────────────────────────────────
App::App(QObject *parent) : QObject(parent) {
  try {
    m_provider = std::unique_ptr<IMediaProvider>(IMediaProvider::create());
  } catch (const std::exception &e) {
    qWarning() << "Failed to initialize Media Provider:" << e.what();
  } catch (...) {
    qWarning() << "Failed to initialize Media Provider due to an unknown exception.";
  }
  m_window = std::make_unique<MainWindow>();

  connect(&m_timer, &QTimer::timeout, this, &App::tick);
  connect(m_window.get(), &MainWindow::settingsChanged, this,
          &App::onSettingsChanged);
}

App::~App() { Utils::writeNullJson(); }

// ── Start
// ─────────────────────────────────────────────────────────────────────
void App::start() {
  loadSettings();

  // Apply loaded settings to the window's checkboxes
  m_window->findChild<QCheckBox *>("dynColor"); // no-op — done via block below
  // Block signals while setting initial state so we don't trigger saves
  m_window->blockSignals(true);
  // Access widgets via property or direct set — we use the public accessors
  // that exist as writable by calling the slots manually (they're just UI
  // updates). Simpler: store the settings in App and let MainWindow read them
  // on show. We call the init-time setter indirectly by using Qt property
  // system:
  m_window->setProperty("_initDynColor", m_dynamicColor);
  m_window->setProperty("_initVisualizer", m_audioVisualizer);
  m_window->blockSignals(false);

  // Actually set checkbox states via findChildren
  auto checkboxes = m_window->findChildren<QCheckBox *>();
  if (checkboxes.size() >= 2) {
    for (auto* cb : checkboxes) cb->blockSignals(true);
    checkboxes[0]->setChecked(m_dynamicColor);
    checkboxes[1]->setChecked(m_audioVisualizer);
    for (auto* cb : checkboxes) cb->blockSignals(false);
  }

  m_window->show();
  m_timer.start(1000);
}

// ── Settings
// ──────────────────────────────────────────────────────────────────
void App::loadSettings() {
  QFile f(Utils::settingsPath());
  if (!f.open(QIODevice::ReadOnly)) {
    qDebug() << "[App] loadSettings: No settings.json found.";
    return;
  }
  auto doc = QJsonDocument::fromJson(f.readAll());
  if (doc.isNull()) {
    qDebug() << "[App] loadSettings: settings.json is invalid.";
    return;
  }
  auto obj = doc.object();
  m_dynamicColor = obj.value("dynamicColor").toBool(false);
  m_audioVisualizer = obj.value("audioVisualizer").toBool(false);
}

void App::saveSettings() {
  QJsonObject obj;
  obj["dynamicColor"] = m_window->dynamicColorEnabled();
  obj["audioVisualizer"] = m_window->audioVisualizerEnabled();

  QFile f(Utils::settingsPath());
  if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

void App::onSettingsChanged() {
  m_dynamicColor = m_window->dynamicColorEnabled();
  m_audioVisualizer = m_window->audioVisualizerEnabled();
  saveSettings();
}

// ── Polling tick
// ──────────────────────────────────────────────────────────────
void App::tick() {
  SongInfo info;
  if (m_provider) {
    info = m_provider->currentSong();
  } else {
    info.status = "closed";
  }

  if (info.status == "closed" || info.status.isEmpty() ||
      info.title.isEmpty()) {
    Utils::writeNullJson();
    m_window->updateSong("", "", info.status.isEmpty() ? "closed" : info.status,
                         {});
    return;
  }

  // Cache thumbnail — only reload when song/art changes
  QString key = info.title + "|" + info.artist + "|" + info.artUrl;
  if (key != m_lastSongKey) {
    m_lastSongKey = key;
    // On Windows, thumbnail is already base64 from the WinRT provider.
    // On Linux, we get an artUrl that we must fetch ourselves.
    if (info.thumbnailB64.isEmpty() && !info.artUrl.isEmpty())
      info.thumbnailB64 = Utils::loadThumbnailAsBase64(info.artUrl);
    m_cachedThumbnailB64 = info.thumbnailB64;
  } else {
    info.thumbnailB64 = m_cachedThumbnailB64;
  }

  Utils::writeJson(info.title, info.artist, info.album, info.status,
                   info.thumbnailB64, m_dynamicColor, m_audioVisualizer);

  m_window->updateSong(info.title, info.artist, info.status, info.thumbnailB64);
}
