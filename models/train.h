#pragma once

/**
 * @file train.h
 * @brief 车次、站点相关的数据结构（成员 D 主维护，成员 E 订票时也会用到）
 */

#include <QDate>
#include <QString>

/** @brief 铁路站点，对应表 stations */
struct Station
{
    int id = 0;
    QString name;  ///< 站名，如 "北京"
};

/**
 * @brief 一趟车次的完整信息，对应表 trains + 部分 seat_inventory 汇总
 *
 * 注意：实际库存按“席别”分多条记录存在 seat_inventory 表里，
 * 查票界面展示时会用 TrainTicketInfo（一行=某车次+某席别）。
 */
struct Train
{
    int id = 0;
    QString trainNo;        ///< 车次号，如 "G101"
    int fromStationId = 0;
    int toStationId = 0;
    QString fromStation;    ///< 出发站名（查询时 JOIN 得到，方便显示）
    QString toStation;      ///< 到达站名
    QString departTime;     ///< 出发时刻 "08:00"
    QString arriveTime;     ///< 到达时刻 "12:36"
    QDate travelDate;       ///< 乘车日期
    double basePrice = 0;   ///< 基准票价
    QString status;         ///< "running" 运行中 / "suspended" 停运
};

/**
 * @brief 查票结果里的一行：某车次、某席别、余票多少
 *
 * 成员 B 的表格、成员 E 的订票界面都会用到这个结构。
 */
struct TrainTicketInfo
{
    int trainId = 0;
    QString trainNo;
    QString fromStation;
    QString toStation;
    QDate travelDate;
    QString departTime;
    QString arriveTime;
    QString seatType;       ///< 如 "二等座"、"硬卧"
    double price = 0;
    int totalSeats = 0;     ///< 该席别总座位
    int remainingSeats = 0; ///< 余票 = total - sold
};
