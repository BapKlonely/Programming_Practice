#pragma once

/*
头文件使用说明：
描述了订单相关的类/结构体，包括订单信息、下单反馈、车辆座次信息。
*/

#include <QDate>
#include <QString>

struct Order //订单信息
{
    int id = 0;
    QString orderNo;        //订单号，应当唯一
    int trainId = 0;
    int operatorId = 0;     // 执行订票/退票操作的管理员 users.id（库字段名为 seller_id）
    QString passengerName;
    QString idCard;
    QString seatType;       //座位类型
    int ticketCount = 1;
    double price = 0;       //实付总价
    QString status;         //订单状态：已支付/已退票
    QString createdAt;      //下单时间

    // 为前端UI设计的展示变量，可以通过查询数据库赋值
    QString trainNo;
    QString fromStation;
    QString toStation;
    QDate travelDate;
};

struct BookResult //用户下单后返回的下单结果
{
    bool ok = false;
    QString orderNo;    //返回用户订单号
    QString message;    //若下单失败，返回失败原因
};

struct TrainSalesStats //某车次列车座次信息
{
    int trainId = 0;
    QString trainNo;
    int totalSeats = 0;
    int soldSeats = 0;
    int remainingSeats = 0;
    double totalRevenue = 0;
};
