#pragma once
#include <QCheckBox>
#include <QLabel>
#include <QMainWindow>
#include <QPixmap>
#include <QSystemTrayIcon>

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  // Called by App every tick to refresh the display
  void updateSong(const QString &title, const QString &artist,
                  const QString &status, const QString &thumbnailB64);
  void setError(const QString &msg);

  // Settings read by App
  bool dynamicColorEnabled() const;
  bool audioVisualizerEnabled() const;

signals:
  void settingsChanged();

protected:
  void closeEvent(QCloseEvent *event) override;
  void changeEvent(QEvent *event) override;

private slots:
  void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
  void onQuitClicked();

private:
  void buildUi();
  void buildTray();
  void restoreWindow();
  void showTrayMessage();

  // ── UI widgets ──────────────────────────────────────────────────────────
  QLabel *m_albumArt = nullptr;
  QLabel *m_title = nullptr;
  QLabel *m_artist = nullptr;
  QLabel *m_status = nullptr;
  QLabel *m_error = nullptr;
  QCheckBox *m_dynColor = nullptr;
  QCheckBox *m_visualizer = nullptr;

  // ── Tray ────────────────────────────────────────────────────────────────
  QSystemTrayIcon *m_tray = nullptr;
  bool m_closing = false; // true only when Quit is clicked
};
