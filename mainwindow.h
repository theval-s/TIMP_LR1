#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemWatcher>
#include <QStringList>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QTimer>
#include <QClipboard>
#include <QMimeData>
#include <QSystemTrayIcon>
#include <QInputDialog>
#include <QRegularExpression>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QCloseEvent>
#include <aclapi.h>
#include <winerror.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_browse_button_clicked();

    void on_show_button_released();

    void on_show_button_pressed();

    void on_change_button_clicked();

    void on_start_button_clicked();

    void on_stop_button_clicked();

private:
    void closeEvent (QCloseEvent *event);
    void changeEvent(QEvent* event);
    Ui::MainWindow *ui;
    void read_from_template();
    void save_to_template();
    void handle_directory_events();
    void handle_clipboard_changes();
    void tray_icon_activated(QSystemTrayIcon::ActivationReason);

    QFileSystemWatcher *watcher;
    QClipboard *clipboard;
};
#endif // MAINWINDOW_H
