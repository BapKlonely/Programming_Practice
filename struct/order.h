#ifndef ORDER_H
#define ORDER_H
#include <QString>
using namespace std;
/**********************/
struct order_information
{
    QString  id;//订单id
    QString account;//用户名
    QString passenger_name;//乘客姓名
    QString passenger_id;//乘客id
    QString train_number;//车次
    QString start_station;//出发站
    QString end_station;//到达站
    QString start_time;//出发时间
    QString end_time;//到达时间
    QString seat_type;//座位类型
    QString seat_number;//座位号
    int ticket_number;//车票数
    double price;//总价
    QString state;//订单状态
    QString order_time;//下单时间
    //显示订单的所有信息
};
#endif
