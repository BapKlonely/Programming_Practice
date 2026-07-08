#pragma once

/**
 * @file TicketService.h
 * @brief 订票 / 退票服务（成员 E 负责，第 3 步实现）
 *
 * 【当前状态】
 * 只有头文件占位，具体逻辑在 TicketService.cpp 中编写。
 * 成员 B 的 UI 应 #include 本文件并调用 bookTicket() / refundTicket()。
 *
 * 【依赖】
 * - Database：读写 orders、seat_inventory、ticket_logs
 * - TrainService：查询车次是否可售（成员 D 提供，未就绪前可用 Stub）
 */

#include "models/order.h"

#include <QString>

class Database;
class TrainService;

class TicketService
{
public:
    TicketService(Database &db, TrainService &trainService);

    // 以下方法第 3 步实现，此处仅声明接口
    BookResult bookTicket(int trainId,
                          const QString &seatType,
                          const QString &passengerName,
                          const QString &idCard,
                          int count,
                          int sellerId);

    bool refundTicket(const QString &orderNo, int sellerId, QString *err);

    double calculatePrice(int trainId, const QString &seatType);

private:
    Database &m_db;
    TrainService &m_trains;
};
