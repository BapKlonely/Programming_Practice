#pragma once

/*
头文件使用说明：
描述了车次相关的类/结构体，包括车次信息、站点信息、实时查询时返回的余量信息。
*/

#include <QDate>
#include <QString>

struct Station //铁路站点信息
{
    int id = 0;     //数据库中利用自增键值编的号
    QString name;   //车站名称
};

struct Train //车次信息
{
    int id = 0;
    QString trainNo;        // 车次号，如 "G101"
    int fromStationId = 0;
    int toStationId = 0;
    QString fromStation;    // 出发站名（查询时 JOIN 得到，方便显示）
    QString toStation;      // 到达站名
    QString departTime;     // 出发时刻
    QString arriveTime;     // 到达时刻
    QDate travelDate;       // 乘车日期
    double basePrice = 0;   // 基准票价
    QString status;         // 车次状态："running" 运行中 / "suspended" 停运
};


struct TrainTicketInfo //余票信息
{
    int trainId = 0;
    QString trainNo;
    QString fromStation;
    QString toStation;
    QDate travelDate;
    QString departTime;
    QString arriveTime;
    QString seatType;       //本系统中暂时只讨论几等座的类别信息
    double price = 0;
    int totalSeats = 0;     //该等级总座位
    int remainingSeats = 0; //该等级剩余座位
};
