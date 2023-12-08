#include "mainwindow.h"
#include <QApplication>
//Т.к. используется WinAPI - сборка не на Windows невозможна
#ifndef _WIN32
#error This can be only compiled on Windows due to usage of windows.h
#endif


int main(int argc, char *argv[])
{
    //Создаём GUI и вызываем его
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowTitle(QString("ТИМП ЛР 1"));
    w.setWindowIcon(QIcon(":/icon/Shield_Icon.png"));
    w.show();
    return a.exec();
}
