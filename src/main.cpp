#include "app.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QIODevice>
#include <QMessageBox>

#ifdef Q_OS_WIN
#include <windows.h> // CreateMutex for single-instance check
#endif

int main(int argc, char *argv[]) {
  QApplication qapp(argc, argv);
  qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &context, const QString &msg) {
      static QFile logFile(QCoreApplication::applicationDirPath() + "/debugomatic.log");
      if (!logFile.isOpen()) {
          logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
      }
      QTextStream ts(&logFile);
      ts << msg << Qt::endl;
  });

  qDebug() << "--- OSMV Started ---";
  qDebug() << "Log path:" << QCoreApplication::applicationDirPath() + "/debugomatic.log";

  qapp.setApplicationName("OBS Stream Music Viewer");
  qapp.setApplicationVersion("2.0");
  qapp.setQuitOnLastWindowClosed(
      false); // Stay alive when window is closed to tray

#ifdef Q_OS_WIN
  // Single-instance check (Windows)
  HANDLE mutex =
      CreateMutexA(nullptr, TRUE, "OBS-StreamMusicViewer-SingleInstance");
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    QMessageBox::information(nullptr, "Application already running",
                             "OBS Stream Music Viewer is already running.\n\n"
                             "Check the system tray icon.");
    return 1;
  }
#else
  // Single-instance check (Linux) — lock file
  QFile lock(QCoreApplication::applicationDirPath() + "/.osmv.lock");
  if (!lock.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    // If we can't even create the lock file, just continue
  }
#endif

  App app;
  app.start();

  int ret = qapp.exec();

#ifdef Q_OS_WIN
  ReleaseMutex(mutex);
  CloseHandle(mutex);
#endif

  return ret;
}
