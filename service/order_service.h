#ifndef ORDER_SERVICE_H
#define ORDER_SERVICE_H
#include <QString>
#include <QList>
#include "struct/order.h"
#include "train_service.h"
#include "user_service.h"
/****************************/
class order_service
{
    private:
    QList<order_information> order_list;//订单列表
    QString order_file_path;//订单文件路径********
    train_service* t_service;//依赖接口
    user_service* u_service;//依赖用户服务
    QString error;//错误信息
    QString generate_order_id();//生成订单id
    bool loadfromfile();//加载订单文件
    bool savetofile();//写入订单文件
    bool check_permission(QString& err);//检查是否有购票权限
    public:
    order_service(const QString& order_file_path,train_service* t_service,user_service* u_service);
    bool bookticket(const QString& account,const QString& train_number,const QString& start_station,const QString& end_station,const QString& strat_date,const QString& start_time,const QString& end_time,const QString& seat_type,int amount,const QString& passenger_name,const QString& passenager_id,QString& err);//订车票（写入完整订单信息）,游客调用时返回 false
    bool cancel_order(const QString& order_id,QString& err);//退票（订单状态改变）,游客调用时返回 false
    QList<order_information> get_order(const QString& account,QString& err);//获取乘客的所有订单信息
    QString geterror() const//获取最后的错误信息
    {
        return error;
    }
};
#endif
