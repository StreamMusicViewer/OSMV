#include "app.h"
#include "discordrpc.h"
#include "mainwindow.h"
#include "mediaprovider.h"
#include "utils.h"

#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

// ── Constructor
// ───────────────────────────────────────────────────────────────
App::App(QObject *parent) : QObject(parent) {
  m_provider = std::unique_ptr<IMediaProvider>(IMediaProvider::create());
  m_window = std::make_unique<MainWindow>();
  m_http = std::make_unique<QNetworkAccessManager>();

  connect(&m_timer, &QTimer::timeout, this, &App::tick);
  connect(m_window.get(), &MainWindow::settingsChanged, this,
          &App::onSettingsChanged);
  connect(m_window.get(), &MainWindow::discordRpcToggled, this,
          &App::onDiscordRpcToggled);
  connect(m_window.get(), &MainWindow::discordClientIdChanged, this,
          &App::onDiscordClientIdChanged);
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
  m_window->setProperty("_initRpc", m_discordRpcEnabled);
  m_window->setProperty("_initClientId", m_discordClientId);
  m_window->blockSignals(false);

  // Actually set checkbox states via findChildren
  auto checkboxes = m_window->findChildren<QCheckBox *>();
  if (checkboxes.size() >= 3) {
    checkboxes[0]->setChecked(m_dynamicColor);
    checkboxes[1]->setChecked(m_audioVisualizer);
    checkboxes[2]->setChecked(m_discordRpcEnabled);
  }
  auto edits = m_window->findChildren<QLineEdit *>();
  if (!edits.isEmpty())
    edits[0]->setText(m_discordClientId);

  syncDiscordRpc();

  m_window->show();
  m_timer.start(1000);
}

// ── Settings
// ──────────────────────────────────────────────────────────────────
void App::loadSettings() {
  QFile f(Utils::settingsPath());
  if (!f.open(QIODevice::ReadOnly))
    return;
  auto doc = QJsonDocument::fromJson(f.readAll());
  if (doc.isNull())
    return;
  auto obj = doc.object();
  m_dynamicColor = obj.value("dynamicColor").toBool(false);
  m_audioVisualizer = obj.value("audioVisualizer").toBool(false);
  m_discordRpcEnabled = obj.value("discordRpc").toBool(false);
  m_discordClientId =
      obj.value("discordClientId").toString("1479531788731809913");
}

void App::saveSettings() {
  QJsonObject obj;
  obj["dynamicColor"] = m_window->dynamicColorEnabled();
  obj["audioVisualizer"] = m_window->audioVisualizerEnabled();
  obj["discordRpc"] = m_window->discordRpcEnabled();
  obj["discordClientId"] = m_window->discordClientId();

  QFile f(Utils::settingsPath());
  if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

void App::onSettingsChanged() {
  m_dynamicColor = m_window->dynamicColorEnabled();
  m_audioVisualizer = m_window->audioVisualizerEnabled();
  saveSettings();
}

void App::onDiscordRpcToggled(bool enabled) {
  m_discordRpcEnabled = enabled;
  saveSettings();
  syncDiscordRpc();
}

void App::onDiscordClientIdChanged(const QString &id) {
  m_discordClientId = id;
  saveSettings();
  if (m_discordRpcEnabled)
    reinitDiscordRpc();
}

// ── Discord
// ───────────────────────────────────────────────────────────────────
void App::syncDiscordRpc() {
  if (m_discordRpcEnabled && !m_discordClientId.isEmpty()) {
    if (!m_discord) {
      m_discord = std::make_unique<DiscordRpc>();
      m_discord->initialize(m_discordClientId);
    }
  } else {
    if (m_discord) {
      m_discord->dispose();
      m_discord.reset();
    }
  }
}

void App::reinitDiscordRpc() {
  if (m_discord) {
    m_discord->dispose();
    m_discord.reset();
  }
  if (m_discordRpcEnabled && !m_discordClientId.isEmpty()) {
    m_discord = std::make_unique<DiscordRpc>();
    m_discord->initialize(m_discordClientId);
  }
}

void App::updateDiscord(const QString &title, const QString &artist,
                        bool isPlaying) {
  if (!m_discordRpcEnabled || !m_discord)
    return;

  // Fetch new cover when song changes
  if (title != m_lastDiscordTitle || artist != m_lastDiscordArtist) {
    m_lastDiscordTitle = title;
    m_lastDiscordArtist = artist;
    m_currentCoverUrl.clear();
    if (!title.isEmpty() && !m_fetchingCover)
      fetchItunesCover(title, artist);
  }
  m_discord->updatePresence(title, artist, isPlaying, m_currentCoverUrl);
}

void App::fetchItunesCover(const QString &title, const QString &artist) {
  m_fetchingCover = true;
  QString query = QUrl::toPercentEncoding(artist + " " + title);
  QUrl url("https://itunes.apple.com/search?term=" + query +
           "&entity=song&limit=1");
  QNetworkRequest req(url);
  req.setHeader(QNetworkRequest::UserAgentHeader, "OSMV/2.0");
  auto *reply = m_http->get(req);
  connect(reply, &QNetworkReply::finished, this, [this, reply]() {
    m_fetchingCover = false;
    if (reply->error() != QNetworkReply::NoError) {
      reply->deleteLater();
      return;
    }
    auto doc = QJsonDocument::fromJson(reply->readAll());
    auto results = doc.object().value("results").toArray();
    if (!results.isEmpty()) {
      QString artwork = results[0].toObject().value("artworkUrl100").toString();
      m_currentCoverUrl = artwork.replace("100x100bb", "512x512bb");
    }
    reply->deleteLater();
  });
}

// ── Polling tick
// ──────────────────────────────────────────────────────────────
void App::tick() {
  SongInfo info = m_provider->currentSong();

  if (info.status == "closed" || info.status.isEmpty() ||
      info.title.isEmpty()) {
    Utils::writeNullJson();
    m_window->updateSong("", "", info.status.isEmpty() ? "closed" : info.status,
                         {});
    if (m_discord)
      updateDiscord({}, {}, false);
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
  updateDiscord(info.title, info.artist, info.isPlaying);
}
