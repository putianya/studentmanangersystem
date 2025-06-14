#include "settings.h"

// 获取Settings类的单例实例（线程安全的懒汉模式）
Settings& Settings::instance()
{
    static Settings instance; // 静态局部变量，保证只创建一次实例

    return instance;
}

// 构造函数：初始化QSettings对象，使用config.ini文件存储配置
Settings::Settings() : settings("config.ini", QSettings::IniFormat)
{}

// 获取数据库路径，默认值为"users.db"
QString Settings::getDatabasePath() const
{
    return settings.value("Database/Path",
                          "S:/Qt/project/StudentManagerSystem/sqlite/StuManSys.db")
           .toString();
}

// 设置数据库路径
void Settings::setDatabasePath(const QString& path)
{
    settings.setValue("Database/Path", path);
}

// 获取是否启用缓存的设置，默认值为true
bool Settings::getCacheEnabled() const
{
    return settings.value("Login/CacheEnabled", true).toBool();
}

// 设置是否启用缓存
void Settings::setCacheEnabled(bool enabled)
{
    settings.setValue("Login/CacheEnabled", enabled);
}

// 获取上次登录的用户名，默认值为空字符串
QString Settings::getLastUser() const
{
    return settings.value("Login/LastUser", "").toString();
}

// 设置上次登录的用户名
void Settings::setLastUser(const QString& user)
{
    settings.setValue("Login/LastUser", user);
}
