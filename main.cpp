#include "mainwindow.h"
#include <QApplication>
#ifndef _WIN32
#error This can be only compiled on Windows due to usage of windows.h
#endif

/*
#include <windows.h>
#include <QMessageBox>
bool is_elevated() {
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if (hToken) {
        CloseHandle(hToken);
    }
    return fRet;
}
*/

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    /*
    if(!is_elevated()){
        QMessageBox not_admin_box;
        not_admin_box.setText("Программа была запущена не от имени администратора, возможны проблемы с работой!");
        not_admin_box.setWindowTitle("Внимание");
        not_admin_box.exec();
    }
    */
    MainWindow w;
    w.setWindowTitle(QString("ТИМП ЛР 1"));
    w.setWindowIcon(QIcon(":/icon/Shield_Icon.png"));
    w.show();
    return a.exec();
}
