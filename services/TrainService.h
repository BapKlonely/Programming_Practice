#pragma once

/**
 * @file TrainService.h
 * @brief 车次查询服务（成员 D 负责，第 3 步由 D 实现；E 测试时可写 Stub）
 *
 * TicketService 订票前会调用 isTrainBookable() / getRemainingSeats()。
 */

#include "models/train.h"

#include <QDate>
#include <QVector>
#include <optional>

class Database;

class TrainService
{
public:
    explicit TrainService(Database &db);

    // 成员 D 实现 CRUD；E 测试订票时至少需要下面三个只读接口
    int getRemainingSeats(int trainId, const QString &seatType);
    bool isTrainBookable(int trainId);
    std::optional<Train> getTrainById(int trainId);

    QVector<TrainTicketInfo> queryRemainingTickets(const QString &from,
                                                   const QString &to,
                                                   const QDate &date);

private:
    Database &m_db;
};
