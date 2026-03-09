#include "discordrpc.h"
#include <QDebug>
#include <QTimerEvent>

// discord-rpc C library
#include <discord_rpc.h>

static constexpr int CLEAR_DELAY_MS = 5000;

DiscordRpc::DiscordRpc(QObject *parent) : QObject(parent) {}

DiscordRpc::~DiscordRpc() { dispose(); }

static void handleDiscordReady(const DiscordUser* user) {
    qDebug() << "[DiscordRPC] CALLBACK: Ready. User:" << user->username << "#" << user->discriminator;
}

static void handleDiscordDisconnected(int errorCode, const char* message) {
    qDebug() << "[DiscordRPC] CALLBACK: Disconnected." << errorCode << ":" << message;
}

static void handleDiscordError(int errorCode, const char* message) {
    qDebug() << "[DiscordRPC] CALLBACK: Error." << errorCode << ":" << message;
}

void DiscordRpc::initialize(const QString &clientId) {
  if (m_initialized)
    return;
  if (clientId.trimmed().isEmpty())
    return;

  m_clientId = clientId.trimmed();
  m_clientIdBytes = m_clientId.toUtf8();
  const char* finalId = m_clientIdBytes.constData();

  memset(&m_handlers, 0, sizeof(m_handlers));
  m_handlers.ready = handleDiscordReady;
  m_handlers.disconnected = handleDiscordDisconnected;
  m_handlers.errored = handleDiscordError;

  qDebug() << "[DiscordRPC] Attempting Initialize with ID [" << finalId << "] Register=1 BufferPtr=" << (void*)finalId;
  Discord_Initialize(finalId, &m_handlers, 1, nullptr);
  
  m_initialized = true;
  m_pollTimerId = startTimer(250); // Poll Discord every 250ms
  qDebug() << "[DiscordRPC] Initialized. TimerID=" << m_pollTimerId;
}

void DiscordRpc::updatePresence(const QString &title, const QString &artist,
                                bool isPlaying, const QString &coverUrl) {
  if (!m_initialized)
    return;

  if (title.isEmpty() && artist.isEmpty()) {
    qDebug() << "[DiscordRPC] Empty track, scheduling clear";
    scheduleClear();
    return;
  }

  // We relaxed this: show "Paused" instead of clearing immediately
  cancelClear();

  DiscordRichPresence presence{};
  QByteArray detailsUtf8 = title.toUtf8();
  
  // State: "by Artist" or "Paused"
  QString stateStr = isPlaying ? ("by " + artist) : "Paused";
  QByteArray stateUtf8 = stateStr.toUtf8();

  // Discord Rich Presence assets must be uploaded to the Developer Portal.
  // We use 'osmv_logo' as the default small icon.
  // We'll only use 'placeholder' as large image if no cover, 
  // and we'll avoid sending actual URLs as keys as they might be rejected.
  QByteArray largeKey = QByteArray("placeholder");
  QByteArray smallKey = QByteArray("osmv_logo");
  
  QByteArray largeText = (title + " - " + artist).toUtf8();
  QByteArray smallText = QByteArray("OBS Stream Music Viewer");

  presence.details = detailsUtf8.constData();
  presence.state = stateUtf8.constData();
  presence.largeImageKey = largeKey.constData();
  presence.largeImageText = largeText.constData();
  presence.smallImageKey = smallKey.constData();
  presence.smallImageText = smallText.constData();

  // If playing, we can show a timer (optional, but nice)
  // presence.startTimestamp = ...

  qDebug() << "[DiscordRPC] Update Presence:" << title << "-" << artist << (isPlaying ? "(Playing)" : "(Paused)");
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
  if (m_pollTimerId != 0) {
    killTimer(m_pollTimerId);
    m_pollTimerId = 0;
  }
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
  qDebug() << "[DiscordRPC] timerEvent received ID:" << event->timerId() << "(PollID is" << m_pollTimerId << ")";
  if (event->timerId() == m_clearTimerId) {
    killTimer(m_clearTimerId);
    m_clearTimerId = 0;
    clearPresence();
  } else if (event->timerId() == m_pollTimerId) {
    if (m_initialized) {
      static int pollCount = 0;
      if (++pollCount % 20 == 0) {
        qDebug() << "[DiscordRPC] Heartbeat: Still polling Discord...";
      }
      Discord_RunCallbacks();
    }
  }
}
