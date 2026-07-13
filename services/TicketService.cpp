#include "services/TicketService.h"

#include "core/database.h"
#include "services/OrderService.h"
#include "services/TrainService.h"

#include <QDateTime>
#include <QRandomGenerator>
#include <QSqlError>
#include <QSqlQuery>

namespace {

//生成唯一订单号，格式 ORD + 时间戳 + 4 位随机数
QString generateOrderNo()
{
    return QStringLiteral("ORD%1%2")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss")))
        .arg(QRandomGenerator::global()->bounded(1000, 9999));
}

/*
    写入一条操作日志
    query:与当前事务共用的QSqlQuery operatorId:执行操作的管理员id
    action:"book"订票或"refund"退票 detail:中文详细信息
    SQL预处理:prepare("... VALUES (?, ?, ?, ?)") 里的 ? 是占位符，防止 SQL 注入；
    addBindValue() 按顺序填入每个 ?，再 exec() 执行。
*/
bool writeTicketLog(QSqlQuery &query, int operatorId, const QString &action, const QString &detail)
{
    query.clear();  // 清空上一条 SQL 的状态，同一条 query 对象可重复使用
    const QString now = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    query.prepare(QStringLiteral("INSERT INTO ticket_logs (seller_id, action, detail, created_at) " "VALUES (?, ?, ?, ?)"));
    query.addBindValue(operatorId);
    query.addBindValue(action);
    query.addBindValue(detail);
    query.addBindValue(now);
    return query.exec();
}
}

TicketService::TicketService(Database &db, TrainService &trainService, OrderService &orders)
    : m_db(db)
    , m_trains(trainService)
    , m_orders(orders)
{
}

BookResult TicketService::bookTicket(int trainId,const QString &seatType,const QString &passengerName,const QString &idCard,int count,int operatorId)
{
    BookResult result;

    // 业务校验：检测可能的错误并报告
    if (passengerName.trimmed().isEmpty() || idCard.trimmed().isEmpty()) {
        result.message = QStringLiteral("请填写乘车人姓名和证件号");
        return result;
    }
    if (count <= 0) {
        result.message = QStringLiteral("购票张数必须大于 0");
        return result;
    }
    if (operatorId <= 0) {
        result.message = QStringLiteral("无效的操作员信息");
        return result;
    }

    if (!m_trains.isTrainBookable(trainId)) {
        result.message = QStringLiteral("该车次已停运，无法购票");
        return result;
    }

    const auto trainOpt = m_trains.getTrainById(trainId);
    if (!trainOpt) {
        result.message = QStringLiteral("车次不存在");
        return result;
    }
    if (m_trains.getRemainingSeats(trainId, seatType) < count) {
        result.message = QStringLiteral("余票不足");//只是简单获取余票信息，并不能防止并发超售
        return result;
    }

    // 开启事务：正式执行操作
    // 事务内任一步失败都要 rollback()，否则连接可能一直处于未提交状态。
    QSqlDatabase conn = m_db.connection();
    if (!conn.transaction()) {
        result.message = QStringLiteral("无法开启事务：%1").arg(conn.lastError().text());
        return result;
    }
    QSqlQuery query(conn);  // 必须绑定到 conn，才能参与该连接上的事务

    // 扣减库存
    // WHERE (total - sold) >= ? 保证不会超售：若余票不够，UPDATE影响第0行，numRowsAffected()==0
    query.clear();
    query.prepare(QStringLiteral(
        "UPDATE seat_inventory "
        "SET sold = sold + ? "
        "WHERE train_id = ? AND seat_type = ? AND (total - sold) >= ?"));
    query.addBindValue(count);
    query.addBindValue(trainId);
    query.addBindValue(seatType);
    query.addBindValue(count);
    if (!query.exec()) {
        conn.rollback();
        result.message = QStringLiteral("扣减库存失败：%1").arg(query.lastError().text());
        return result;
    }
    if (query.numRowsAffected() == 0) {
        conn.rollback();
        result.message = QStringLiteral("余票不足");
        return result;
    }

    // 插入订单
    const double unitPrice = calculatePrice(trainId, seatType);
    const double totalPrice = unitPrice * count;
    const QString orderNo = generateOrderNo();
    const QString now = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    query.clear();
    query.prepare(QStringLiteral(
        "INSERT INTO orders "
        "(order_no, train_id, seller_id, passenger_name, id_card, seat_type, ticket_count, price, status, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 'paid', ?)"));
    query.addBindValue(orderNo);
    query.addBindValue(trainId);
    query.addBindValue(operatorId);
    query.addBindValue(passengerName.trimmed());
    query.addBindValue(idCard.trimmed());
    query.addBindValue(seatType);
    query.addBindValue(count);
    query.addBindValue(totalPrice);
    query.addBindValue(now);
    if (!query.exec()) {
        conn.rollback();
        result.message = QStringLiteral("写入订单失败：%1").arg(query.lastError().text());
        return result;
    }

    // 订单日志
    const QString logDetail = QStringLiteral("订单 %1 车次 %2 %3 张 %4")
                                  .arg(orderNo, trainOpt->trainNo)
                                  .arg(count)
                                  .arg(seatType);
    if (!writeTicketLog(query, operatorId, QStringLiteral("book"), logDetail)) {
        conn.rollback();
        result.message = QStringLiteral("写入操作日志失败：%1").arg(query.lastError().text());
        return result;
    }

    // 提交事务
    if (!conn.commit()) {
        conn.rollback();
        result.message = QStringLiteral("提交事务失败：%1").arg(conn.lastError().text());
        return result;
    }

    result.ok = true;
    result.orderNo = orderNo;
    return result;
}

//退票：查订单 → 事务内改状态 → 恢复库存 → 写日志
bool TicketService::refundTicket(const QString &orderNo, int operatorId, QString *err)
{
    if (orderNo.trimmed().isEmpty()) {
        if (err) {
            *err = QStringLiteral("订单号不能为空");
        }
        return false;
    }

    // 通过 OrderService 查单（与查单界面同一套 JOIN 逻辑，避免重复 SQL）
    const auto orderOpt = m_orders.getOrderById(orderNo.trimmed());
    if (!orderOpt) {
        if (err) {
            *err = QStringLiteral("未找到该订单");
        }
        return false;
    }

    const Order &order = *orderOpt;
    const int trainId = order.trainId;
    const QString seatType = order.seatType;
    const int ticketCount = order.ticketCount;

    if (order.status == QStringLiteral("refunded")) {
        if (err) {
            *err = QStringLiteral("订单已退票");
        }
        return false;
    }

    QSqlDatabase conn = m_db.connection();
    if (!conn.transaction()) {
        if (err) {
            *err = QStringLiteral("无法开启事务：%1").arg(conn.lastError().text());
        }
        return false;
    }

    QSqlQuery query(conn);

    // 把订单标为 refunded；WHERE status='paid' 防止重复退票
    query.clear();
    query.prepare(QStringLiteral(
        "UPDATE orders SET status = 'refunded' WHERE order_no = ? AND status = 'paid'"));
    query.addBindValue(orderNo.trimmed());
    if (!query.exec() || query.numRowsAffected() == 0) {
        conn.rollback();
        if (err) {
            *err = QStringLiteral("更新订单状态失败");
        }
        return false;
    }

    // 恢复库存：sold 减回去
    query.clear();
    query.prepare(QStringLiteral(
        "UPDATE seat_inventory "
        "SET sold = sold - ? "
        "WHERE train_id = ? AND seat_type = ? AND sold >= ?"));
    query.addBindValue(ticketCount);
    query.addBindValue(trainId);
    query.addBindValue(seatType);
    query.addBindValue(ticketCount);
    if (!query.exec() || query.numRowsAffected() == 0) {
        conn.rollback();
        if (err) {
            *err = QStringLiteral("恢复库存失败：%1").arg(query.lastError().text());
        }
        return false;
    }

    const QString logDetail = QStringLiteral("退票 %1，恢复 %2 张 %3")
                                  .arg(orderNo)
                                  .arg(ticketCount)
                                  .arg(seatType);
    if (!writeTicketLog(query, operatorId, QStringLiteral("refund"), logDetail)) {
        conn.rollback();
        if (err) {
            *err = QStringLiteral("写入操作日志失败：%1").arg(query.lastError().text());
        }
        return false;
    }

    if (!conn.commit()) {
        conn.rollback();
        if (err) {
            *err = QStringLiteral("提交事务失败：%1").arg(conn.lastError().text());
        }
        return false;
    }

    return true;
}

//自动浮动票价
double TicketService::calculatePrice(int trainId, const QString &seatType)
{
    const auto trainOpt = m_trains.getTrainById(trainId);
    if (!trainOpt) {
        return 0.0;
    }

    const auto inv = m_trains.getSeatInventory(trainId, seatType);
    if (!inv || inv->total <= 0) {
        return trainOpt->basePrice;
    }

    const double soldRatio = static_cast<double>(inv->sold) / inv->total;
    return trainOpt->basePrice * (1.0 + 0.2 * soldRatio);
}
