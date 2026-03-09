#include "discordrpc.h"
#include <QDebug>
#include <QTimerEvent>

// discord-rpc C library
#include <discord_rpc.h>

static constexpr int CLEAR_DELAY_MS = 5000;

DiscordRpc::DiscordRpc(QObject *parent) : QObject(parent) {}

DiscordRpc::~DiscordRpc() { dispose(); }

void DiscordRpc::initialize(const QString &clientId) {
  if (m_initialized)
    return;
  if (clientId.trimmed().isEmpty())
    return;

  m_clientId = clientId.trimmed();

  DiscordEventHandlers handlers{};
  handlers.ready = [](const DiscordUser *user) {
    qDebug() << "[DiscordRPC] Ready, user:" << user->username;
  };
  handlers.errored = [](int code, const char *msg) {
    qDebug() << "[DiscordRPC] Error" << code << msg;
  };
  handlers.disconnected = [](int code, const char *msg) {
    qDebug() << "[DiscordRPC] Disconnected" << code << msg;
  };

  Discord_Initialize(m_clientId.toUtf8().constData(), &handlers, 1, nullptr);
  m_initialized = true;
  qDebug() << "[DiscordRPC] Initialized with client_id=" << m_clientId;
}

void DiscordRpc::updatePresence(const QString &title, const QString &artist,
                                bool isPlaying, const QString &coverUrl) {
  if (!m_initialized)
    return;

  if (title.isEmpty() && artist.isEmpty()) {
    scheduleClear();
    return;
  }
  if (!isPlaying) {
    scheduleClear();
    return;
  }

  cancelClear();

  DiscordRichPresence presence{};
  QByteArray detailsUtf8 = title.toUtf8();
  QByteArray stateUtf8 = ("by " + artist).toUtf8();
  QByteArray largeKey =
      coverUrl.isEmpty() ? QByteArray("placeholder") : coverUrl.toUtf8();
  QByteArray largeText = (title + " - " + artist).toUtf8();
  QByteArray smallKey = QByteArray("osmv_logo");
  QByteArray smallText = QByteArray("OBS Stream Music Viewer");

  presence.details = detailsUtf8.constData();
  presence.state = stateUtf8.constData();
  presence.largeImageKey = largeKey.constData();
  presence.largeImageText = largeText.constData();
  presence.smallImageKey = smallKey.constData();
  presence.smallImageText = smallText.constData();

  Discord_UpdatePresence(&presence);
  Discord_RunCallbacks();
}

void DiscordRpc::clearPresence() {
  if (!m_initialized)
    return;
  Discord_ClearPresence();
  Discord_RunCallbacks();
}

void DiscordRpc::dispose() {
  cancelClear();
  if (m_initialized) {
    clearPresence();
    Discord_Shutdown();
    m_initialized = false;
  }
}

void DiscordRpc::scheduleClear() {
  if (m_clearTimerId != 0)
    return; // already scheduled
  m_clearTimerId = startTimer(CLEAR_DELAY_MS);
}

void DiscordRpc::cancelClear() {
  if (m_clearTimerId != 0) {
    killTimer(m_clearTimerId);
    m_clearTimerId = 0;
  }
}

void DiscordRpc::timerEvent(QTimerEvent *event) {
  if (event->timerId() == m_clearTimerId) {
    killTimer(m_clearTimerId);
    m_clearTimerId = 0;
    clearPresence();
  }
}
