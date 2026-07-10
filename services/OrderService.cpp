#include "services/OrderService.h"

#include "core/database.h"

#include <QSqlError>
#include <QSqlQuery>

namespace {

//将SQL查询结果行转换为Order对象
Order rowToOrder(const QSqlQuery &query)
{
    Order order;
    order.id = query.value(QStringLiteral("id")).toInt();
    order.orderNo = query.value(QStringLiteral("order_no")).toString();
    order.trainId = query.value(QStringLiteral("train_id")).toInt();
    order.operatorId = query.value(QStringLiteral("seller_id")).toInt();
    order.passengerName = query.value(QStringLiteral("passenger_name")).toString();
    order.idCard = query.value(QStringLiteral("id_card")).toString();
    order.seatType = query.value(QStringLiteral("seat_type")).toString();
    order.ticketCount = query.value(QStringLiteral("ticket_count")).toInt();
    order.price = query.value(QStringLiteral("price")).toDouble();
    order.status = query.value(QStringLiteral("status")).toString();
    order.createdAt = query.value(QStringLiteral("created_at")).toString();

    order.trainNo = query.value(QStringLiteral("train_no")).toString();
    order.fromStation = query.value(QStringLiteral("from_name")).toString();
    order.toStation = query.value(QStringLiteral("to_name")).toString();
    order.travelDate = query.value(QStringLiteral("travel_date")).toDate();
    return order;
}

// SQL查询语句，用于获取订单及相关信息
const char *kOrderSelectSql = R"SQL(
    SELECT o.id, o.order_no, o.train_id, o.seller_id,
           o.passenger_name, o.id_card, o.seat_type, o.ticket_count,
           o.price, o.status, o.created_at,
           t.train_no, s1.name AS from_name, s2.name AS to_name, t.travel_date
    FROM orders o
    JOIN trains t ON o.train_id = t.id
    JOIN stations s1 ON t.from_station_id = s1.id
    JOIN stations s2 ON t.to_station_id = s2.id
)SQL";
}

OrderService::OrderService(Database &db): m_db(db){}//构造函数，初始化数据库连接

//按订单号查询
std::optional<Order> OrderService::getOrderById(const QString &orderNo)
{
    const QString key = orderNo.trimmed();//清洗输入，去除前后的空格
    if (key.isEmpty() || !m_db.isOpen()) {//若输入为空或数据库未打开，则返回空值
        return std::nullopt;
    }

    QSqlQuery query(m_db.connection());//创建一个SQL查询对象

    //准备SQL查询语句，使用预处理语句防止SQL注入
    query.prepare(QString::fromUtf8(kOrderSelectSql) + QStringLiteral(" WHERE o.order_no = :orderNo"));
    query.bindValue(QStringLiteral(":orderNo"), key);

    //执行查询，如果失败或查询为空则返回空值
    if (!query.exec()) {
        return std::nullopt;
    }
    if (!query.next()) {
        return std::nullopt;
    }
    return rowToOrder(query);
}

//按乘客信息查询，和getOrderById类似，只是查询条件不同
QVector<Order> OrderService::searchOrdersByPassenger(const QString &name)
{
    QVector<Order> result;
    const QString key = name.trimmed();
    if (key.isEmpty() || !m_db.isOpen()) {
        return result;
    }

    QSqlQuery query(m_db.connection());
    query.prepare(QString::fromUtf8(kOrderSelectSql)
                  + QStringLiteral(" WHERE o.passenger_name LIKE :name "
                                   "ORDER BY o.created_at DESC"));
    query.bindValue(QStringLiteral(":name"), QStringLiteral("%%1%").arg(key));

    if (!query.exec()) {
        return result;
    }

    while (query.next()) {
        result.append(rowToOrder(query));
    }
    return result;
}
