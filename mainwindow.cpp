#include "mainwindow.h"
#include "./ui_mainwindow.h"

QString filePath = NULL;
bool is_active = false;
QList<QFileInfo> directory_contents;
QList<QRegularExpression> mask_list;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    //Базовая изначальная настройка и инициализация GUI
    filePath = QCoreApplication::applicationDirPath();
    ui->setupUi(this);
    ui->lineEdit->setText(filePath);
    ui->stop_button->setDisabled(true);

    //Добавим функционал иконки в трее
    QSystemTrayIcon *trayIcon = new QSystemTrayIcon(this);
    QMenu *trayIconMenu = new QMenu(this);

    //Создаем действия, связанные с иконкой в трее
    QAction *restoreAction = new QAction("Открыть", this);
    connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);
    QAction *quitAction = new QAction("Закрыть", this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    //Добавляем эти действия в контекстое меню для иконки в трее
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayIconMenu);

    //Чтобы приложение открывалось при двойном клике присоединим сигнал trayIconActivated
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::tray_icon_activated);

    //Присваиваем картинку приложения иконке в трее
    trayIcon->setIcon(QIcon(":/icon/Shield_Icon.png"));
    trayIcon->show();

    read_from_template();
}

MainWindow::~MainWindow()
{
    //Деструктор главного окна приложения
    delete ui;
}

//Функции, отвечающие за слоты сигналов кнопок
void MainWindow::on_browse_button_clicked()
{
    //Открываем диалог выбора директории
    QString tempDirName = QFileDialog::getExistingDirectory(this, "Choose Directory", filePath,QFileDialog::ShowDirsOnly);
    if(tempDirName.isEmpty()){
        //QMessageBox::information(this, "isNULL", QString("Папка не была выбрана"));
        return;
    } else {
        //Меняем папку работы
        filePath=tempDirName;
        ui->lineEdit->setText(filePath);
        read_from_template();
    }

}

//Показ/скрытие пароля при нажатии
void MainWindow::on_show_button_pressed()
{
    ui->password->setEchoMode(QLineEdit::Normal);
}
void MainWindow::on_show_button_released()
{
    ui->password->setEchoMode(QLineEdit::Password);
}

//Изменение пароля, если он уже был задан в файле
void MainWindow::on_change_button_clicked()
{
    ui->password->setReadOnly(false);
    ui->change_button->setDisabled(true);
    ui->show_button->setEnabled(true);
}

//Запуск защиты
void MainWindow::on_start_button_clicked()
{
    //Отключаем кнопки и изменяющиеся поля
    is_active=true;
    save_to_template();
    ui->browse_button->setDisabled(true);
    ui->lineEdit->setDisabled(true);
    ui->textEdit->setDisabled(true);
    ui->start_button->setDisabled(true);
    ui->stop_button->setEnabled(true);
    ui->show_button->setDisabled(true);

    //Обрабатываем список заданных имён и масок
    QStringList name_list = ui->textEdit->toPlainText().split(QRegularExpression("[\\n]"), Qt::SkipEmptyParts);
    mask_list.resize(name_list.count());
    for(qsizetype i = 0; i < name_list.size();i++){
        //qDebug() << name_list[i];
        //Преобразуем маски в RegExp и сохраняем их
        name_list[i].replace(".", "\\.").replace("?", ".").replace("*", ".*");
        mask_list[i] = QRegularExpression(name_list[i]);
        //qDebug() << mask_list[i].pattern();
    }
    //Проходимся по всем файлам в папке
    QDirIterator it(filePath, QDirIterator::NoIteratorFlags);
    while (it.hasNext()){
        it.next();
        QString entry =  it.fileName();
        if(entry == ".." || entry == ".") continue;
        for(QRegularExpression &mask : mask_list){
            //qDebug() << entry;-
            //Если файл подходит по одной из масок - ограничиваем к нему доступ
            if(mask.match(entry).hasMatch()){
                PACL pDACL=NULL;
                EXPLICIT_ACCESS ea;
                ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
                //Задаём необходимую политику доступа
                ea.grfAccessPermissions = GENERIC_READ;
                ea.grfAccessMode = SET_ACCESS;
                ea.grfInheritance= NO_INHERITANCE;
                ea.Trustee.TrusteeForm=TRUSTEE_IS_NAME;
                ea.Trustee.ptstrName= const_cast<LPWSTR>(L"EVERYONE");
                //И применяем её
                if(SetEntriesInAcl(1, &ea, NULL, &pDACL) != ERROR_SUCCESS){
                    QMessageBox::critical(this, "Error!", QString("SetEntriesInAcl() for template.tbl failed! Error code:") + QString::number(GetLastError()));
                    QApplication::quit();
                }
                qDebug() << it.filePath();
                if(SetNamedSecurityInfoA(LPSTR(it.filePath().toStdString().c_str()),SE_FILE_OBJECT, DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION, NULL, NULL, pDACL, NULL)!=ERROR_SUCCESS){
                    //Если выполнилось неуспешно а LastError == 0 - значит просто нет прав
                    if(GetLastError() != 0){
                    QMessageBox::critical(this, "Error", "Ошибка в SetNamedSecurityInfo. Error:" + QString::number(GetLastError()));
                    qDebug() << (SetNamedSecurityInfoA(LPSTR(it.filePath().toStdString().c_str()),SE_FILE_OBJECT, DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION, NULL, NULL, pDACL, NULL)!=ERROR_SUCCESS);
                    //QApplication::quit();
                    }
                }
                LocalFree(pDACL);
            }
        }
    }
    //Сохраняем текущее содержимое папки, чтобы потом сравнивать с изменениями
    QDir directory(filePath);
    directory_contents = directory.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    //Присоединяем сигналы от FileSystemWatcher и Clipboard к функциям
    watcher = new QFileSystemWatcher(this);
    watcher->addPath(filePath);
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::handle_directory_events);

    clipboard = QGuiApplication::clipboard();
    connect(clipboard,&QClipboard::dataChanged,this, &MainWindow::handle_clipboard_changes);
}

//Остановка защиты
void MainWindow::on_stop_button_clicked()
{
    //Если был задан пароль - не выключать, пока не введут пароль правильно
    if(!ui->password->text().isEmpty()){
        for(;;){
            QString input_password = QInputDialog::getText(this, "Остановка", "Для остановки программы введите пароль");
            if (input_password == ui->password->text()) break;
            else if(input_password.isEmpty()) return;
        }
    }
    //Если ввели пароль правильно - возвращаем активное состояние кнопкам и выключаем защиту
    is_active=false;
    QString full_file_path = filePath+"/"+"template.tbl";
    QFile file(filePath+"/"+"template.tbl");
    ui->browse_button->setEnabled(true);
    ui->lineEdit->setEnabled(true);
    ui->textEdit->setEnabled(true);
    ui->start_button->setEnabled(true);
    ui->stop_button->setDisabled(true);
    //Возвращаем файлу права создателя файла
    PACL pDACL=NULL;
    EXPLICIT_ACCESS ea;
    //Аналогично, выставляем необходимые права
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = GENERIC_ALL;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance= NO_INHERITANCE;
    ea.Trustee.TrusteeForm=TRUSTEE_IS_NAME;
    ea.Trustee.ptstrName= const_cast<LPWSTR>(L"CREATOR OWNER");
    //qDebug() << full_file_path;
    if(SetEntriesInAcl(1, &ea, NULL, &pDACL) != ERROR_SUCCESS){
        QMessageBox::critical(this, "Error!", QString("SetEntriesInAcl() for template.tbl failed! Error code:") + QString::number(GetLastError()));
        QApplication::quit();
    }
    if(SetNamedSecurityInfoA(LPSTR(full_file_path.toStdString().c_str()),SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pDACL, NULL)!=ERROR_SUCCESS){
        QMessageBox::critical(this, "Error", "Ошибка в SetNamedSecurityInfo. Error code:" + QString::number(GetLastError()));
        QApplication::quit();
    }
    LocalFree(pDACL);

    //Снимаем ограничения прав с файлов, на которые задали их
    QDirIterator it(filePath, QDirIterator::NoIteratorFlags);
    while (it.hasNext()){
        it.next();
        QString entry =  it.fileName();
        if(entry == ".." || entry == ".") continue;
        for(QRegularExpression &mask : mask_list){
            //qDebug() << entry;
            if(mask.match(entry).hasMatch()){
                pDACL=NULL;
                EXPLICIT_ACCESS ea;
                //Аналогично, выставляет права
                ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
                ea.grfAccessPermissions = GENERIC_ALL;
                ea.grfAccessMode = SET_ACCESS;
                ea.grfInheritance= NO_INHERITANCE;
                ea.Trustee.TrusteeForm=TRUSTEE_IS_NAME;
                ea.Trustee.ptstrName= const_cast<LPWSTR>(L"CREATOR OWNER");
                qDebug() << it.filePath();
                if(SetEntriesInAcl(1, &ea, NULL, &pDACL) != ERROR_SUCCESS){
                    QMessageBox::critical(this, "Error!", QString("SetEntriesInAcl() for template.tbl failed! Error code:") + QString::number(GetLastError()));
                    QApplication::quit();
                }
                if(SetNamedSecurityInfoA(LPSTR(it.filePath().toStdString().c_str()),SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pDACL, NULL)!=ERROR_SUCCESS){
                    //Если выполнилось неуспешно а LastError == 0 - значит просто нет прав
                    if(GetLastError()!= 0){
                    QMessageBox::critical(this, "Error", "Ошибка в SetNamedSecurityInfo. Error: "+QString::number(GetLastError()));
                    QApplication::quit();
                    }
                }
                LocalFree(pDACL);
            }
        }
    }
    //Отключаем слоты, чтобы сигналы перестали вызывать функции
    watcher->removePath(filePath);
    disconnect(watcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::handle_directory_events);
    delete(watcher);
    disconnect(clipboard,&QClipboard::dataChanged,this, &MainWindow::handle_clipboard_changes);
}
//////////////////////////////////////////////////////////////////

void MainWindow::read_from_template(){
    QFile file(filePath+"/"+"template.tbl");
    //QMessageBox::information(this, "Info", file.fileName());
    //Если файла не существует - предлагаем создать
    if(!file.exists()){
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "File does not exist", "Файл template.tbl не существует в рабочей директории. Создать его? При нажатии на 'нет' программа закроется.",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            //Создаём
            file.open(QIODevice::WriteOnly);
            file.close();
        } else {
            //Если пользователь решил не создавать файл - закрываем приложение.
            qDebug() << "No was clicked";
            QTimer::singleShot(0, qApp, &QCoreApplication::quit);
        }
    } else {
        //Если файл уже был - считываем что там находится
        if(!file.open(QIODevice::ReadOnly)){
            QMessageBox::critical(this, "Error", "Не удалось открыть файл! Программа будет закрыта");
            QTimer::singleShot(0, qApp, &QCoreApplication::quit);
        }
        ui->textEdit->clear();
        QTextStream in(&file);
        QString line = in.readLine();
        //В первой строчке всегда хранится хеш пароля, поэтому если он не пустой - проверяем пароль
        if(!line.isEmpty()){
            ui->textEdit->setReadOnly(true);
            ui->password->setReadOnly(true);
            qint8 br_protection = 3;
            QByteArray template_hash = QByteArray::fromHex(line.toUtf8());
            while((br_protection--) > 0) {
                QString input_password = QInputDialog::getText(this, "Info", "В первой строке шаблона содержится хеш пароля. Чтобы изменить шаблон введите пароль");
                QByteArray input_hash = QCryptographicHash::hash(input_password.toUtf8(),QCryptographicHash::Sha256);
                if (template_hash == input_hash){
                    //Если пароль ввели правильно - снимаем ограничения
                    ui->textEdit->setReadOnly(false);
                    ui->password->setText(input_password);
                    ui->change_button->setEnabled(true);
                    ui->show_button->setDisabled(true);
                    break;
                }
            }
            //Если слишком много раз ввели пароль неправильно - программу можно будет запустить с уже готовым шаблоном
            if(br_protection <= 0) QMessageBox::information(this, "Too many attempts", "Вы ошиблись с вводом пароля 3 раза. Вы не сможете изменить этот файл");
        }
        while(!in.atEnd()){
            line = in.readLine();
            if(line.startsWith('#')) continue;
            ui->textEdit->append(line);
            //Все считанные строчки добавляем в текущий список в элементе textEdit
        }
        file.close();
    }
}

void MainWindow::save_to_template(){
    QString full_file_path = filePath+"/"+"template.tbl";
    QFile file(full_file_path);
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
    if(!ui->password->text().isEmpty()){
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
    ea.Trustee.ptstrName= const_cast<LPWSTR>(L"ADMINISTRATORS");
    if(SetEntriesInAcl(1, &ea, NULL, &pDACL) != ERROR_SUCCESS){
        QMessageBox::critical(this, "Error!", QString("SetEntriesInAcl() for template.tbl failed! Error code:") + QString::number(GetLastError()));
        QApplication::quit();
    }
    //qDebug() << full_file_path;
    if(SetNamedSecurityInfoA(LPSTR(full_file_path.toStdString().c_str()),SE_FILE_OBJECT, DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION, NULL, NULL, pDACL, NULL)!=ERROR_SUCCESS){
        QMessageBox::critical(this, "Error", "Ошибка в SetNamedSecurityInfo. Error code:" + QString::number(GetLastError()));
        QApplication::quit();
    }
    LocalFree(pDACL);
}

void MainWindow::handle_directory_events(){
    //В случае если зафиксировано изменение в папке (изменение файла, создание/удаление), проверяем если это файл заданный маской
    qDebug() << "Directory change in filePath";
    QDir directory(filePath);
    QList<QFileInfo> new_contents = directory.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    qDebug() << directory_contents.count() << " " << new_contents.count() << Qt::endl;
    //if(directory_contents.size() <= new_contents.size()){} //поскольку могло измениться количество файлов, не относящихся к маске - проверять необходимо всегда
    qsizetype offset = 0;
    //сравниваем все файлы подряд друг с другом
    for(qsizetype i = 0; i < directory_contents.count(); i++){
        if(directory_contents.at(i).fileName() != new_contents.at(i+offset).fileName()){
            //т.к. могло уменьшиться количество файлов, не относящихся к списку защищаемых - учитываем случай когда файлов стало меньше чем было до
            if(new_contents.count()<directory_contents.count() && i == new_contents.count()) break;
            for(auto &mask : mask_list){
                //если файл совпадает по маске с защищаемым, и является новым - удаляем, т.к. нельзя создавать
                if(mask.match(new_contents.at(i+offset).fileName()).hasMatch()){
                    QFile file(new_contents.at(i+offset).absoluteFilePath());
                    file.remove(); //если не получается удалить файл - вероятнее всего у программы нет на это прав, поэтому опустим обработку ошибок
                    offset++;
                }
            }
        }
    }
    if(offset == 0 && directory_contents.size() != new_contents.size()){
        for(qsizetype i = directory_contents.count(); i < new_contents.count(); i++){
            for(auto &mask: mask_list){
                qDebug() << new_contents.at(i+offset).fileName();
                if(mask.match(new_contents.at(i+offset).fileName()).hasMatch()){
                    QFile file(new_contents.at(i+offset).absoluteFilePath());
                    file.remove(); //если не получается удалить файл - вероятнее всего у программы нет на это прав
                }
            }
        }
    }

}

void MainWindow::handle_clipboard_changes(){
    //Если были изменения в буфере обмена - проверяем, не наши ли файлы там
    //qDebug() << "clipboard changed" << Qt::endl;
    const QMimeData *mimeData = clipboard->mimeData();
    if(mimeData->hasUrls()){
        QList<QUrl> urlList = mimeData->urls();
        for (const QUrl &url : urlList) {
            if (url.isLocalFile()) {
                QFileInfo file_info(url.toLocalFile());
                if(file_info.filePath() != filePath) break; //если это файл не в нашей папке - не проверяем
                for(QRegularExpression &mask : mask_list){
                    //qDebug() << entry;-
                    //Если файл подходит по одной из масок - ограничиваем к нему доступ
                    if(mask.match(file_info.fileName()).hasMatch()){
                        clipboard->clear();
                    }

                }
                qDebug() << "File copied:" << url.toLocalFile();
            }

        }
    }
}
void MainWindow::closeEvent (QCloseEvent *event){
    if(is_active){
        //если защита активна - перехватываем попытки закрыть приложение
        QMessageBox::information(this, "Warning","Вы не можете закрыть приложение пока работает защита. Остановите защиту, введя пароль, если он есть.");
        event->ignore();
    } else event->accept();
}

void MainWindow::changeEvent(QEvent* event)
{
    //При попытке свернуть - сворачиваем приложение в трей
    if (event->type() == QEvent::WindowStateChange && isMinimized())
    {
        this->hide();
        event->ignore();
    }
}

void MainWindow::tray_icon_activated(QSystemTrayIcon::ActivationReason reason){
    //Если иконку в трее
    if(reason == QSystemTrayIcon::DoubleClick){
        this->showNormal();
    }
}
