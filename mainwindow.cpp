#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QString>
#include <QStringList>
#include <QFileDialog>
#include <QDir>
#include <QDirIterator>
#include <QTimer>
#include <QInputDialog>
#include <QRegularExpression>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QList>
#include <QCloseEvent>
#include <QFileSystemWatcher>
#include <aclapi.h>
#include <winerror.h>

QString filePath = NULL;
bool is_active = false;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    filePath = QCoreApplication::applicationDirPath();
    ui->setupUi(this);
    ui->lineEdit->setText(filePath);
    ui->stop_button->setDisabled(true);
    read_from_template();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_browse_button_clicked()
{
    QString tempDirName = QFileDialog::getExistingDirectory(this, "Choose Directory", filePath,QFileDialog::ShowDirsOnly);
    if(tempDirName.isEmpty()){
        //QMessageBox::information(this, "isNULL", QString("Папка не была выбрана"));
        return;
    } else {
        filePath=tempDirName;
        read_from_template();
    }

}

void MainWindow::on_show_button_pressed()
{
    ui->password->setEchoMode(QLineEdit::Normal);
}

void MainWindow::on_show_button_released()
{
    ui->password->setEchoMode(QLineEdit::Password);
}

void MainWindow::on_change_button_clicked()
{
    ui->password->setReadOnly(false);
    ui->change_button->setDisabled(true);
    ui->show_button->setEnabled(true);
}

void MainWindow::on_start_button_clicked()
{
    is_active=true;
    save_to_template();
    ui->browse_button->setDisabled(true);
    ui->lineEdit->setDisabled(true);
    ui->textEdit->setDisabled(true);
    ui->start_button->setDisabled(true);
    ui->stop_button->setEnabled(true);
    ui->show_button->setDisabled(true);
    QDirIterator it(filePath, QDirIterator::NoIteratorFlags);
    QStringList name_list = ui->textEdit->toPlainText().split(QRegularExpression("[\\n]"), Qt::SkipEmptyParts);
    QList<QRegularExpression> mask_list(name_list.size());
    for(qsizetype i = 0; i < name_list.size();i++){
        //qDebug() << name_list[i];
        name_list[i].replace(".", "\\.").replace("?", ".").replace("*", ".*");
        mask_list[i] = QRegularExpression(name_list[i]);
        //qDebug() << mask_list[i].pattern();
    }
    while (it.hasNext()){
        it.next();
        QString entry =  it.fileName();
        if(entry == ".." || entry == ".") continue;
        for(QRegularExpression mask : mask_list){
           //qDebug() << entry;
            if(mask.match(entry).hasMatch()){
               PACL pDACL=NULL;
               EXPLICIT_ACCESS ea;
               ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
               ea.grfAccessPermissions = GENERIC_ALL;
               ea.grfAccessMode = SET_ACCESS;
               ea.grfInheritance= NO_INHERITANCE;
               ea.Trustee.TrusteeForm=TRUSTEE_IS_NAME;
               ea.Trustee.ptstrName= L"ADMINISTRATORS";
               if(SetEntriesInAcl(1, &ea, NULL, &pDACL) != ERROR_SUCCESS){
                   QMessageBox::critical(this, "Error!", QString("SetEntriesInAcl() for template.tbl failed! Error code:") + QString::number(GetLastError()));
                   QApplication::quit();
               }
               if(SetNamedSecurityInfoA(LPSTR(entry.toStdString().c_str()),SE_FILE_OBJECT, DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION, NULL, NULL, pDACL, NULL)!=ERROR_SUCCESS){
                   QMessageBox::critical(this, "Error", "Ошибка в SetNamedSecurityInfo");
                   QApplication::quit();
               }
               LocalFree(pDACL);
           }
        }
    }
    //TODO: ADD WATCHER
}


void MainWindow::on_stop_button_clicked()
{
    if(!ui->password->text().isEmpty()){
        for(;;){
        QString input_password = QInputDialog::getText(this, "Остановка", "Для остановки программы введите пароль");
            if (input_password == ui->password->text()) break;
            else if(input_password.isEmpty()) return;
        }
    }
    is_active=false;
    QFile file(filePath+"/"+"template.tbl");
    ui->browse_button->setEnabled(true);
    ui->lineEdit->setEnabled(true);
    ui->textEdit->setEnabled(true);
    ui->start_button->setEnabled(true);
    ui->stop_button->setDisabled(true);
    //Возвращаем файлу права создателя файла
    PACL pDACL=NULL;
    EXPLICIT_ACCESS ea;
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = GENERIC_ALL;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance= NO_INHERITANCE;
    ea.Trustee.TrusteeForm=TRUSTEE_IS_NAME;
    ea.Trustee.ptstrName= L"CREATOR OWNER";
    if(SetEntriesInAcl(1, &ea, NULL, &pDACL) != ERROR_SUCCESS){
        QMessageBox::critical(this, "Error!", QString("SetEntriesInAcl() for template.tbl failed! Error code:") + QString::number(GetLastError()));
        QApplication::quit();
    }
    if(SetNamedSecurityInfoA(LPSTR(file.fileName().toStdString().c_str()),SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pDACL, NULL)!=ERROR_SUCCESS){
        QMessageBox::critical(this, "Error", "Ошибка в SetNamedSecurityInfo");
        QApplication::quit();
    }
    LocalFree(pDACL);


    QDirIterator it(filePath, QDirIterator::NoIteratorFlags);
    QStringList name_list = ui->textEdit->toPlainText().split(QRegularExpression("[\\n]"), Qt::SkipEmptyParts);
    QList<QRegularExpression> mask_list(name_list.size());
    for(qsizetype i = 0; i < name_list.size();i++){
        //qDebug() << name_list[i];
        name_list[i].replace(".", "\\.").replace("?", ".").replace("*", ".*");
        mask_list[i] = QRegularExpression(name_list[i]);
        //qDebug() << mask_list[i].pattern();
    }

    while (it.hasNext()){
        it.next();
        QString entry =  it.fileName();
        if(entry == ".." || entry == ".") continue;
        for(QRegularExpression mask : mask_list){
           //qDebug() << entry;
            if(mask.match(entry).hasMatch()){
               pDACL=NULL;
               EXPLICIT_ACCESS ea;
               ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
               ea.grfAccessPermissions = GENERIC_ALL;
               ea.grfAccessMode = SET_ACCESS;
               ea.grfInheritance= NO_INHERITANCE;
               ea.Trustee.TrusteeForm=TRUSTEE_IS_NAME;
               ea.Trustee.ptstrName= L"CREATOR OWNER";
               if(SetEntriesInAcl(1, &ea, NULL, &pDACL) != ERROR_SUCCESS){
                   QMessageBox::critical(this, "Error!", QString("SetEntriesInAcl() for template.tbl failed! Error code:") + QString::number(GetLastError()));
                   QApplication::quit();
               }
               if(SetNamedSecurityInfoA(LPSTR(entry.toStdString().c_str()),SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pDACL, NULL)!=ERROR_SUCCESS){
                   QMessageBox::critical(this, "Error", "Ошибка в SetNamedSecurityInfo");
                   QApplication::quit();
               }
               LocalFree(pDACL);
           }
        }
    }
    //TODO: WATCHER
}
//////////////////////////////////////////////////////////////////

void MainWindow::read_from_template(){
    QFile file(filePath+"/"+"template.tbl");
    //QMessageBox::information(this, "Info", file.fileName());
    if(!file.exists()){
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "File does not exist", "Файл template.tbl не существует в рабочей директории. Создать его? При нажатии на 'нет' программа закроется.",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            file.open(QIODevice::WriteOnly);
            file.close();
        } else {
            qDebug() << "No was clicked";
            QTimer::singleShot(0, qApp, &QCoreApplication::quit);
        }
    } else {
        if(!file.open(QIODevice::ReadOnly)){
            QMessageBox::critical(this, "Error", "Не удалось открыть файл! Программа будет закрыта");
            QTimer::singleShot(0, qApp, &QCoreApplication::quit);
        }
        ui->textEdit->clear();
        QTextStream in(&file);
        QString line = in.readLine();
        if(!line.isEmpty()){
            ui->textEdit->setReadOnly(true);
            ui->password->setReadOnly(true);
            qint8 br_protection = 3;
            QByteArray template_hash = QByteArray::fromHex(line.toUtf8());
            while((br_protection--) > 0) {
                QString input_password = QInputDialog::getText(this, "Info", "В первой строке шаблона содержится хеш пароля. Чтобы изменить шаблон введите пароль");
                QByteArray input_hash = QCryptographicHash::hash(input_password.toUtf8(),QCryptographicHash::Sha256);
                if (template_hash == input_hash){
                    ui->textEdit->setReadOnly(false);
                    ui->password->setText(input_password);
                    ui->change_button->setEnabled(true);
                    ui->show_button->setDisabled(true);
                    break;
                }
            }
            if(br_protection <= 0) QMessageBox::information(this, "Too many attempts", "Вы ошиблись с вводом пароля 3 раза. Вы не сможете изменить этот файл");
        }
        while(!in.atEnd()){
            line = in.readLine();
            if(line.startsWith('#')) continue;
            ui->textEdit->append(line);
        }
        file.close();
    }
}

void MainWindow::save_to_template(){
    QFile file(filePath+"/"+"template.tbl");
    if(!file.exists()){
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "File does not exist", "Файл template.tbl не существует в рабочей директории. Создать его? При нажатии на 'нет' программа закроется.",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            file.open(QIODevice::WriteOnly);
            file.close();
        } else {
            //qDebug() << "No was clicked";

        }
    }
    //Открываем файл
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QMessageBox::critical(this, "Error", "Не удалось открыть файл! Программа будет закрыта");
        QCoreApplication::quit();
    }
    QTextStream out(&file);
    //Сохраняем пароль в виде хеша SHA256 в первой строке
    if(!(ui->password->isReadOnly()) && !(ui->password->text().isEmpty())){
        QByteArray hash = QCryptographicHash::hash(ui->password->text().toUtf8(), QCryptographicHash::Sha256);
        out << hash.toHex() << Qt::endl;
    } else out << Qt::endl;
    out << "#First line is autogenerated. Modifying it might make this file uneditable by program" << Qt::endl;
    //Выводим текущие имена и шаблоны в файл
    out << ui->textEdit->toPlainText();
    file.flush();
    file.close();
    //Выставляем соответствующие ограничения на доступ к файлу (доступ к template.tbl имеют только администраторы пока программа работает)
    PACL pDACL=NULL;
    EXPLICIT_ACCESS ea;
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = GENERIC_ALL;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance= NO_INHERITANCE;
    ea.Trustee.TrusteeForm=TRUSTEE_IS_NAME;
    ea.Trustee.ptstrName= L"ADMINISTRATORS";
    if(SetEntriesInAcl(1, &ea, NULL, &pDACL) != ERROR_SUCCESS){
        QMessageBox::critical(this, "Error!", QString("SetEntriesInAcl() for template.tbl failed! Error code:") + QString::number(GetLastError()));
        QApplication::quit();
    }
    if(SetNamedSecurityInfoA(LPSTR(file.fileName().toStdString().c_str()),SE_FILE_OBJECT, DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION, NULL, NULL, pDACL, NULL)!=ERROR_SUCCESS){
        QMessageBox::critical(this, "Error", "Ошибка в SetNamedSecurityInfo");
        QApplication::quit();
    }
    LocalFree(pDACL);
}

void MainWindow::closeEvent (QCloseEvent *event){
    if(is_active){
        QMessageBox::information(this, "Warning","Вы не можете закрыть приложение пока работает защита. Остановите защиту, введя пароль, если он есть.");
        event->ignore();
    } else event->accept();
}



