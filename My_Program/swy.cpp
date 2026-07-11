#include "swy.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
bool op::connectDateBase(const QString& dbPath){
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);
    if (!db.open())
    {
        err = db.lastError().text();
        return false;
    }
    err = "";
    return true;
}
bool op::addUser(int ID, QString account, QString username, QString password, QString phone){
    QSqlQuery query;
    query.prepare("INSERT user_info(ID)");
    return true;
}
QString op::getError() const{
    return err;
}