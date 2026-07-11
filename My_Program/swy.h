#ifndef SWY_H
#define SWY_H
#include <QString>
using namespace std;
class op{
private:
    QString err;
public:
    bool connectDateBase(const QString& dbPath = "stock_user_new");
    bool addUser(int ID, QString account, QString username, QString password, QString phone);
    QString getError() const;
};

#endif // SWY_H
