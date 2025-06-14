#include "databasemanager.h"
#include <QDebug>
#include <QSqlError>

DataBaseManager &DataBaseManager::instance()
{
    static DataBaseManager instance;
    return instance;
}

void DataBaseManager::closeDatabase()
{
    if(db.isOpen())
    {
        db.close();
    }
}

bool DataBaseManager::openDatabase(const QString &path)
{
    db.setDatabaseName(path);
    if(!db.open()){
        qDebug()<<"无法打开数据库："<<db.lastError().text();
        return false;
    }
    return true;
}

QString DataBaseManager::getDatabasePath() const
{
    return dbPath;
}

void DataBaseManager::setDatabasePath(const QString &path)
{
    if(path!= dbPath){//如果路径和上一次的不一样，即新开一个数据库
        dbPath=path;
        closeDatabase();
        openDatabase(path);

    }

}

DataBaseManager::~DataBaseManager()
{
    closeDatabase();
}

DataBaseManager::DataBaseManager(QObject *parent)
    : QObject{parent}
{
    db= QSqlDatabase::addDatabase("QSQLITE");
    openDatabase(dbPath);
}

