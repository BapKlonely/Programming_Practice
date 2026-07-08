#pragma once

/**
 * @file order.h
 * @brief 订单相关的数据结构（成员 E 主维护）
 *
 * 【BookResult 是什么？】
 * 订票接口的返回值。成功时 ok=true 并带 orderNo；
 * 失败时 ok=false 并带中文 message，UI 直接弹窗显示即可。
 */

#include <QDate>
#include <QString>

/** @brief 一条购票订单，对应表 orders */
struct Order
{
    int id = 0;
    QString orderNo;        ///< 业务订单号，如 ORD20260707001
    int trainId = 0;
    int sellerId = 0;       ///< 哪位售票员卖的票，来自 User.id
    QString passengerName;
    QString idCard;
    QString seatType;
    int ticketCount = 1;
    double price = 0;       ///< 实付总价
    QString status;         ///< "paid" 已支付 / "refunded" 已退票
    QString createdAt;

    // 以下字段方便 UI 展示，可从 trains 表 JOIN 查询得到
    QString trainNo;
    QString fromStation;
    QString toStation;
    QDate travelDate;
};

/** @brief bookTicket() 的返回结果 */
struct BookResult
{
    bool ok = false;
    QString orderNo;
    QString message;  ///< 失败时的中文原因，如 "余票不足"
};

/** @brief 某车次售票统计（P1/P2 统计页用，先定义好结构） */
struct TrainSalesStats
{
    int trainId = 0;
    QString trainNo;
    int totalSeats = 0;
    int soldSeats = 0;
    int remainingSeats = 0;
    double totalRevenue = 0;
};
