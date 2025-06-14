#ifndef SETTINGS_H
#define SETTINGS_H
#include <QSettings>
#include <QString>
class Settings {
public:

    static Settings& instance();
    QSettings      & getQSettings() {
        return settings;
    } // 直接暴露底层的 QSettings 对象，允许外部代码绕过类的封装，直接操作配置文件

    QString getDatabasePath() const;
    void    setDatabasePath(const QString& path);
    bool    getCacheEnabled() const;
    void    setCacheEnabled(bool enabled);
    QString getLastUser() const;
    void    setLastUser(const QString& user);

private:

    Settings();
    QSettings settings;
};

#endif // SETTINGS_H
