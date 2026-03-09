#pragma once
#include <QObject>
#include <QString>
#include <functional>
#include <discord_rpc.h>

/// Discord Rich Presence service.
/// Wraps the discord-rpc C library.
/// Mirrors the logic of DiscordRpcService.cs and discord_rpc_service.py.
class DiscordRpc : public QObject {
  Q_OBJECT
public:
  explicit DiscordRpc(QObject *parent = nullptr);
  ~DiscordRpc();

  void initialize(const QString &clientId);
  void updatePresence(const QString &title, const QString &artist,
                      bool isPlaying, const QString &coverUrl = {});
  void clearPresence();
  void dispose();

  bool isInitialized() const { return m_initialized; }

private:
  void scheduleClear();
  void cancelClear();

  bool m_initialized = false;
  QString m_clientId;
  int m_clearTimerId = 0; // QObject timer id for delayed clear
  int m_pollTimerId = 0;  // QObject timer id for polling Discord IPC
  QByteArray m_clientIdBytes; 
  DiscordEventHandlers m_handlers{};
  // iTunes cover fetch callback
  QString m_lastTitle;
  QString m_lastArtist;

protected:
  void timerEvent(QTimerEvent *event) override;
};
