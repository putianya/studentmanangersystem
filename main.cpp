#include "mainwindow.h"
#include "databasemanager.h"
#include "logindialog.h"
#include <QApplication>
#include <Qfile>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile styleFile(":/style/1.qss");

    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QString(styleFile.readAll());
        a.setStyleSheet(styleSheet); // 应用样式表
        styleFile.close();
    } else {
        qWarning() << "打开失败" << styleFile.errorString();
    }

    DataBaseManager::instance();

    LoginDialog loginDlg;

    if (loginDlg.exec() == QDialog::Accepted) {
        MainWindow w;
        w.show();
        return a.exec();
    }
    return 0;
}
