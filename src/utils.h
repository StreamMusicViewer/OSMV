#pragma once
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

namespace Utils {

/// Writes current_song.json next to the executable.
void writeJson(const QString &title, const QString &artist,
               const QString &album, const QString &status,
               const QString &thumbnailB64, bool dynamicColor,
               bool audioVisualizer);

/// Writes a null JSON (no song playing).
void writeNullJson();

/// Load an image from a file:// or http(s):// URL and return base64.
/// Synchronous — uses QFile for local, QNetworkAccessManager for remote.
QString loadThumbnailAsBase64(const QString &url);

/// Encode raw bytes to base64 string.
QString toBase64(const QByteArray &data);

/// Path to current_song.json (next to executable).
QString jsonOutputPath();

/// Path to settings.json (next to executable).
QString settingsPath();

} // namespace Utils
