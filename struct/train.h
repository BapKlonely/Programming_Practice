#ifndef TRAIN_H
#define TRAIN_H
#include <QString>
using namespace std;
/******************************/
struct seat_information
{
    double price=0;
    int remaing_seat=0;
};//给出一种座位的价格以及余票信息
struct train_information
{
    QString id;//车次
    QString start_station;//始发站
    QString end_station;//终点站
    QString start_day;//发车日期
    QString start_time;//发车时间
    QString end_time;//到达时间
    seat_information hard_seat;//硬座
    seat_information soft_seat;//软座
    seat_information hard_sleep;//硬卧
    seat_information soft_sleep;//软卧
    //显示车次的所有信息
};
#endif
