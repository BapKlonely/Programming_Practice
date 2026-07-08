#ifndef DATASTORE_H
#define DATASTORE_H

#include <QString>
#include <vector>
#include <QDate>
#include <QDir>


struct Train{
    QString trainId;
    QString start;
    QString destination;
    QDate startDate;
    QString timeStart;
    QDate ArriveDate;
    QString timeEnd;
    QString seatType;
    int totalSeat;
    int restSeat;
    double price;
};

struct Order{
    QString id;
    QString username;
    QString phone;
    QString train;
    QString createdAt;
    int count;
};

struct User{
    std::vector<Order> orders;
    QString username;
    QString password;
    QString name;
    QString phone;
    bool isAdmin=false;
};

class DataStore
{
public:
    DataStore(const QString &path);
    QString datafile(const QString &path);
    void registerAccount(const QString &username,const QString &password);
private:
    const QString m_path;
    std::vector<Order> m_order;
    std::vector<User> m_user;
    std::vector<Train> m_train;
    static void OrderFromJson();
    static void TrainFromJson();
    static void UserFromJson();
    static void OrderToJson();
    static void TrainToJson();
    static void UserToJson();
};

#endif // DATASTORE_H
