#include "systemsettingswidget.h"
#include "ui_systemsettingswidget.h"
#include <QLineEdit>
#include <QGridLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QSqlError>
#include <QTextEdit>
#include <QLabel>
#include <QFileDialog>
#include "settings.h"
#include <QMessageBox>
#include <QSqlQuery>
#include <QCryptographicHash>

#include "databasemanager.h"
SystemSettingsWidget::SystemSettingsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SystemSettingsWidget)
{
    setFixedSize(400, 600);
    ui->setupUi(this);
    createUI();
    loadSettings();
}

SystemSettingsWidget::~SystemSettingsWidget()
{
    delete ui;
}

void SystemSettingsWidget::createUI()
{
    dbPathEdit = new QLineEdit(this);
    browseBtn = new QPushButton("浏览...", this);
    oldPwdEdit = new QLineEdit(this);
    newPwdEdit = new QLineEdit(this);
    confirmPwdEdit = new QLineEdit(this);
    cacheCheckBox = new QCheckBox("记住登录信息", this);
    saveBtn = new QPushButton("保存", this);
    versionInfoEdit = new QTextEdit(this);

    oldPwdEdit->setEchoMode(QLineEdit::Password);
    newPwdEdit->setEchoMode(QLineEdit::Password);
    confirmPwdEdit->setEchoMode(QLineEdit::Password);

    versionInfoEdit->setPlainText(
        "教学管理系统 1.0\n 开发环境：QT C++ 6.8，Qt Creator 15.0.0，Win10");
    versionInfoEdit->setReadOnly(true);

    mainLayout = new QGridLayout(this);
    mainLayout->addWidget(            new QLabel("数据库路径:", this), 0, 0);
    mainLayout->addWidget(     dbPathEdit,                        0, 1);
    mainLayout->addWidget(      browseBtn,                        0, 2);
    mainLayout->addWidget(            new QLabel("旧密码:", this),   1, 0);
    mainLayout->addWidget(     oldPwdEdit,                        1, 1, 1, 2);
    mainLayout->addWidget(            new QLabel("新密码:", this),   2, 0);
    mainLayout->addWidget(     newPwdEdit,                        2, 1, 1, 2);
    mainLayout->addWidget(            new QLabel("确认密码:", this),  3, 0);
    mainLayout->addWidget( confirmPwdEdit,                        3, 1, 1, 2);
    mainLayout->addWidget(  cacheCheckBox,                        4, 0, 1, 3);
    mainLayout->addWidget(        saveBtn,                        5, 1, 1, 2);
    mainLayout->addWidget(versionInfoEdit,                        6, 0, 1, 3);
    setLayout(mainLayout);

    connect(browseBtn, &QPushButton::clicked, this,
            &SystemSettingsWidget::browseDatabasePath);

    connect(  saveBtn, &QPushButton::clicked, this,
              &SystemSettingsWidget::saveSettings);
}

// 加载当前设置
void SystemSettingsWidget::loadSettings()
{
    dbPathEdit->setText(Settings::instance().getDatabasePath());
    cacheCheckBox->setChecked(Settings::instance().getCacheEnabled());
}

//选择数据库存储位置
void SystemSettingsWidget::browseDatabasePath()
{
    QString path = QFileDialog::getSaveFileName(
        this,
        "选择数据库文件",
        "",
        "SQLite Databases (*.db *.sqlite)"
        );

    if (!path.isEmpty()) dbPathEdit->setText(path);
}

// 修改密码
void SystemSettingsWidget::updatePassword()
{
    if (!validatePasswordChange()) return;

    QString newHash = QString(QCryptographicHash::hash(
                                  newPwdEdit->text().toUtf8(),
                                  QCryptographicHash::Sha256
                                  ).toHex());

    QSqlQuery query;
    query.prepare("UPDATE users SET password = ? WHERE username = ?");
    query.addBindValue( newHash);
    query.addBindValue(Settings::instance().getLastUser());

    if (!query.exec()) {
        QMessageBox::critical(this, "错误", "密码更新失败: " + query.lastError().text());
        return;
    }
    QMessageBox::information(this, "提示", "密码更新成功");
}

// 密码有效性验证
bool SystemSettingsWidget::validatePasswordChange()
{
    if (newPwdEdit->text() != confirmPwdEdit->text()) {
        QMessageBox::warning(this, "错误", "新密码与确认密码不一致");
        return false;
    }
    QString currentUser = Settings::instance().getLastUser();

    if (currentUser.isEmpty()) {
        QMessageBox::warning(this, "错误", "未找到当前用户");
        return false;
    }
    QSqlQuery query;
    query.prepare("SELECT password FROM users WHERE username = ?");
    query.addBindValue(currentUser);

    if (!query.exec() || !query.next()) {
        QMessageBox::critical(this, "错误", "数据库查询失败: " + query.lastError().text());
        return false;
    }
    QString storedHash = query.value(0).toString();
    QString inputHash = QString(QCryptographicHash::hash(
                                    oldPwdEdit->text().toUtf8(),
                                    QCryptographicHash::Sha256
                                    ).toHex());

    if (storedHash != inputHash) {
        QMessageBox::warning(this, "错误", "旧密码不正确");
        return false;
    }
    return true;
}

// 保存系统设置
void SystemSettingsWidget::saveSettings()
{
    QString newDbPath = dbPathEdit->text();

    Settings::instance().setDatabasePath(newDbPath);
    Settings::instance().setCacheEnabled(cacheCheckBox->isChecked());

    if (!newPwdEdit->text().isEmpty()) {
        updatePassword();
    }

    // 检查数据库文件是否存在，若不存在则尝试创建
    QFile file(newDbPath);

    if (!file.exists()) {
        if (!DataBaseManager::instance().openDatabase(newDbPath)) {
            QMessageBox::critical(this, "错误", "无法创建新的数据库文件");
            return;
        }
    }

    if (newDbPath != Settings::instance().getDatabasePath()) {
        QMessageBox::information(this, "提示", "数据库路径修改将在重启后生效");
    }
}
