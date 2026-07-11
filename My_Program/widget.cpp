#include "widget.h"
#include "./ui_widget.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("D:/vsc/Virtual_Stock_Trading_Platform/My_Program/stock_user_new.db");

    if (!db.open()) {
        qDebug() << "打不开数据库！" << db.lastError().text();
        return;
    }
    qDebug() << "数据库打开成功！";

    QSqlQuery query;
}

Widget::~Widget()
{
    delete ui;
}
