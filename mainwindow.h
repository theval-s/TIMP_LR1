#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
    Ui::MainWindow *ui;
    void read_from_template();
    void save_to_template();
};
#endif // MAINWINDOW_H
