#include "utils.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace Utils {

QString jsonOutputPath() {
  return QDir::currentPath() + "/current_song.json";
}

QString settingsPath() {
  return QDir::currentPath() + "/settings.json";
}

QString toBase64(const QByteArray &data) {
  return QString::fromLatin1(data.toBase64());
}

void writeJson(const QString &title, const QString &artist,
               const QString &album, const QString &status,
               const QString &thumbnailB64, bool dynamicColor,
               bool audioVisualizer) {
  QJsonObject obj;
  obj["title"] = title;
  obj["artist"] = artist;
  obj["album"] = album;
  obj["thumbnail"] = thumbnailB64;
  obj["status"] = status;
  obj["dynamicColor"] = dynamicColor;
  obj["audioVisualizer"] = audioVisualizer;
  obj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

  QFile f(jsonOutputPath());
  if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
  }
}

void writeNullJson() {
  QFile f(jsonOutputPath());
  if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    f.write("null\n");
  }
}

QString loadThumbnailAsBase64(const QString &url) {
  if (url.isEmpty())
    return {};

  if (url.startsWith("file://")) {
    // Local file
    QString path = QUrl(url).toLocalFile();
    QFile f(path);
    if (f.open(QIODevice::ReadOnly))
      return toBase64(f.readAll());
    return {};
  }

  // Remote URL — synchronous fetch via QNetworkAccessManager
  QNetworkAccessManager mgr;
  QUrl qurl{url};
  QNetworkRequest req{qurl};
  req.setHeader(QNetworkRequest::UserAgentHeader, "OSMV/2.0");
  QEventLoop loop;
  QNetworkReply *reply = mgr.get(req);

  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();
  if (reply->error() == QNetworkReply::NoError) {
    QString result = toBase64(reply->readAll());
    reply->deleteLater();
    return result;
  }
  reply->deleteLater();
  return {};
}

} // namespace Utils
