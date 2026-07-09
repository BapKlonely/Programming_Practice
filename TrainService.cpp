#ifndef TRAINSERVICE_H
#define TRAINSERVICE_H

#include <QString>
#include <QVector>
#include <QDate>
#include <optional>
#include <QSqlDatabase>
#include "../models/train.h"

class TrainService {
public:
    explicit TrainService(QSqlDatabase &db);

    // 管理员接口
    bool addTrain(const Train &train, QString *err);
    bool updateTrain(const Train &train, QString *err);
    bool deleteTrain(int trainId, QString *err);          
    bool setTrainStatus(int trainId, const QString &status, QString *err);
    bool configureSeats(int trainId, const QString &seatType, int total, QString *err);

    // 查询接口
    QVector<Train> searchTrains(const QString &trainNo, const QString &from, const QDate &date);
    QVector<TrainTicketInfo> queryRemainingTickets(const QString &from, const QString &to, const QDate &date);

    // 内部校验接口
    int getRemainingSeats(int trainId, const QString &seatType);
    bool isTrainBookable(int trainId);   
    std::optional<Train> getTrainById(int trainId);

private:
    QSqlDatabase &m_db; 
};

#endif // TRAINSERVICE_H