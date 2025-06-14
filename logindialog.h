#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

namespace Ui {
class LoginDialog;
}

class QLineEdit;
class QPushButton;

class LoginDialog : public QDialog {
    Q_OBJECT

public:

    explicit LoginDialog(QDialog *parent = nullptr);
    ~LoginDialog();

private:

    Ui::LoginDialog *ui;

    void    checkAndCreateInitialUser();
    void    on_loginButton_clicked();
    QString hashPassword(const QString& password);
    QString encryptPassword(const QString& password);
    bool    validateUser(const QString& username,
                         const QString& password);
    void    saveCredentials(const QString& username,
                            const QString& password);
    bool    loadCredentials(QString& username,
                            QString& password);
    QString decryptPassword(const QString& encryptedPassword);

    QLineEdit *usernameLineEdit;
    QLineEdit *passwordLineEdit;
    QPushButton *loginButton;
    QPushButton *cancelButton;
};

#endif // LOGINDIALOG_H
