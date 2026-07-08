#pragma once

#include <QDate>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

struct Train {
    QString number;
    QString from;
    QString to;
    QDate date;
    QString departTime;
    QString arriveTime;
    QString seatType;
    double price = 0.0;
    int totalSeats = 0;
    int leftSeats = 0;
    QDate arriveDate;
};

struct User {
    QString username;
    QString password;
    QString name;
    QString phone;
    bool admin = false;
};

struct Order {
    QString id;
    QString username;
    QString trainNumber;
    QString from;
    QString to;
    QDate date;
    QString departTime;
    QString passengerName;
    QString passengerId;
    int count = 1;
    double amount = 0.0;
    QString status;
    QString createdAt;
};

class DataStore {
public:
    explicit DataStore(const QString &basePath);

    bool load();
    bool saveAll() const;

    QVector<Train> trains() const;
    QVector<User> users() const;
    QVector<Order> orders() const;

    bool registerUser(const User &user, QString *error);
    const User *findUser(const QString &username) const;

    bool upsertTrain(const Train &train, QString *error);
    bool removeTrain(const QString &number, QString *error);
    Train *findTrain(const QString &number);
    const Train *findTrain(const QString &number) const;

    bool createOrder(const QString &username,
                     const Train &train,
                     const QString &passengerName,
                     const QString &passengerId,
                     int count,
                     Order *created,
                     QString *error);
    bool refundOrder(const QString &orderId, QString *error);

private:
    QString m_basePath;
    QVector<Train> m_trains;
    QVector<User> m_users;
    QVector<Order> m_orders;

    bool loadTrains();
    bool loadUsers();
    bool loadOrders();
    bool openDatabase();
    bool createTables();
    bool saveTrains() const;
    bool saveUsers() const;
    bool saveOrders() const;
    void seedIfEmpty();

    QString databaseFile() const;
    QSqlDatabase m_db;
};
