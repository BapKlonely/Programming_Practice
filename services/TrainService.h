#pragma once

/**
 * @file TrainService.h
 * @brief 车次 / 站点 / 余票查询服务（成员 D）
 *
 * 界面层（成员 B）与 TicketService（成员 E）只通过本类公开方法访问车次数据。
 * 库存扣减不在本类暴露，由 TicketService 在事务内完成。
 */

#include "models/train.h"

#include <QDate>
#include <QString>
#include <QVector>

#include <optional>

class Database;

class TrainService
{
public:
    explicit TrainService(Database &db);

    // 管理员 CRUD
    bool addTrain(const Train &train, QString *err);
    bool updateTrain(const Train &train, QString *err);
    bool deleteTrain(int trainId, QString *err);
    bool setTrainStatus(int trainId, const QString &status, QString *err);
    bool configureSeats(int trainId, const QString &seatType, int total, QString *err);

    // 查询 — B 的查票 / 管理界面
    QVector<Train> searchTrains(const QString &trainNo, const QString &from, const QDate &date);
    QVector<TrainTicketInfo> queryRemainingTickets(const QString &from, const QString &to, const QDate &date);

    // 供 E 订票前校验
    int getRemainingSeats(int trainId, const QString &seatType);
    bool isTrainBookable(int trainId);
    std::optional<Train> getTrainById(int trainId);

private:
    Database &m_db;
};
