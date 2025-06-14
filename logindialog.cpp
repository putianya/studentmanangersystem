#include "logindialog.h"
#include "ui_logindialog.h"
#include <QLabel>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include  "settings.h"
#include <QMessageBox>

// 登录对话框的构造函数，负责初始化UI界面和连接信号槽
LoginDialog::LoginDialog(QDialog *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    checkAndCreateInitialUser(); // 检查数据库是否为空，若为空则插入初始用户
    setWindowTitle("教学管理系统");    // 设置窗口标题
    setWindowIcon(QIcon(":/ico/NEWSAT.ICO"));
    setFixedSize(260, 180);

    // 创建控件
    QLabel *usernameLabel = new QLabel("用户名:", this);
    QLabel *passwordLabel = new QLabel("密   码:", this);
    usernameLineEdit = new QLineEdit(this);
    passwordLineEdit = new QLineEdit(this);
    passwordLineEdit->setEchoMode(QLineEdit::Password); // 密码输入模式
    loginButton = new QPushButton("登录", this);
    cancelButton = new QPushButton("取消", this);

    // 布局
    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(usernameLabel,    0, 0);
    mainLayout->addWidget(usernameLineEdit, 0, 1);
    mainLayout->addWidget(passwordLabel,    1, 0);
    mainLayout->addWidget(passwordLineEdit, 1, 1);
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(loginButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout, 2, 0, 1, 2); // 跨两列
    setLayout(mainLayout);

    // 连接信号和槽
    connect(loginButton,  &QPushButton::clicked, this,
            &LoginDialog::on_loginButton_clicked);
    connect(cancelButton, &QPushButton::clicked, this, &LoginDialog::reject);

    // 尝试加载缓存的登录信息
    QString cachedUsername, cachedPassword;

    if (Settings::instance().getCacheEnabled()) {
        if (loadCredentials(cachedUsername, cachedPassword)) {
            usernameLineEdit->setText(cachedUsername);
            passwordLineEdit->setText(cachedPassword);
        }
    }
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

// 检查数据库是否为空，若为空则创建初始管理员账户的函数
void LoginDialog::checkAndCreateInitialUser() {
    const QString initialUsername = "admin"; // 初始用户名和密码
    const QString initialPassword = "admin123";
    QSqlQuery     query;

    query.exec("SELECT COUNT(*) FROM users");

    if (query.next() && (query.value(0).toInt() == 0)) { // 检查 users 表是否为空,表为空，插入初始用户
        QString hashedInitialPassword = hashPassword(initialPassword);
        query.prepare(
            "INSERT INTO users (username, password) VALUES (:username, :password)");
        query.bindValue(":username",       initialUsername);
        query.bindValue(":password", hashedInitialPassword);

        if (!query.exec()) qDebug() << "插入初始用户失败:" << query.lastError().text();
    }
}

// 登录按钮点击事件处理函数，验证用户并处理登录逻辑
void LoginDialog::on_loginButton_clicked() {
    QString username = usernameLineEdit->text();
    QString password = passwordLineEdit->text();

    if (validateUser(username, password)) {
        saveCredentials(username, password);        // 登录成功，保存登录信息
        Settings::instance().setLastUser(username); // 将当前登录的用户名保存到 Settings 中
        accept();                                   // 关闭对话框
    }
    else QMessageBox::warning(this, "登录失败", "用户名或密码错误。");
}

// 验证用户名和密码是否匹配的函数
bool LoginDialog::validateUser(const QString& username, const QString& password) {
    QString   hashedPassword = hashPassword(password);
    QSqlQuery query;

    query.prepare(
        "SELECT * FROM users WHERE username = :username AND password = :password");
    query.bindValue(":username",       username);
    query.bindValue(":password", hashedPassword);

    if (!query.exec()) {
        qDebug() << "查询错误:" << query.lastError().text();
        return false;
    }
    return query.next();
}

// 保存用户登录凭证到配置文件的函数
void LoginDialog::saveCredentials(const QString& username,
                                  const QString& password) {
    // 存储用户名和加密后的密码
    Settings::instance().getQSettings().setValue("username", username);
    QString encryptedPassword = encryptPassword(password);
    Settings::instance().getQSettings().setValue("password", encryptedPassword);
}

// 使用SHA256算法对密码进行哈希加密的函数
QString LoginDialog::hashPassword(const QString& password) {
    QByteArray passwordBytes = password.toUtf8();
    QByteArray hashBytes = QCryptographicHash::hash(passwordBytes,
                                                    QCryptographicHash::Sha256);

    return QString(hashBytes.toHex());
}

// 从配置文件加载缓存的登录凭证
bool LoginDialog::loadCredentials(QString& username, QString& password) {
    username = Settings::instance().getQSettings().value("username").toString();
    QString encryptedPassword = Settings::instance().getQSettings().value(
        "password").toString();

    if (!username.isEmpty() && !encryptedPassword.isEmpty()) { // 如果用户名和加密后的密码不为空，则解密密码
        password = decryptPassword(encryptedPassword);
        return true;
    }
    return false;
}

// ============加密存储在本地配置文件的密码，存储在数据库的密码是用哈希加密===========

// 使用异或加密算法对密码进行加密的函数（用于本地存储）
const QByteArray encryptionKey = "your_encryption_key";         // 加密和解密的密钥
QString LoginDialog::encryptPassword(const QString& password) { // 加密函数
    QByteArray passwordBytes = password.toUtf8();
    QByteArray encryptedBytes;

    for (int i = 0; i < passwordBytes.size(); ++i) encryptedBytes.append(
            passwordBytes[i] ^ encryptionKey[i % encryptionKey.size()]);
    return encryptedBytes.toBase64();
}

// 解密函数
QString LoginDialog::decryptPassword(const QString& encryptedPassword) {
    QByteArray encryptedBytes =
        QByteArray::fromBase64(encryptedPassword.toUtf8());
    QByteArray decryptedBytes;

    for (int i = 0; i < encryptedBytes.size(); ++i) decryptedBytes.append(
            encryptedBytes[i] ^ encryptionKey[i % encryptionKey.size()]);
    return QString::fromUtf8(decryptedBytes);
}
