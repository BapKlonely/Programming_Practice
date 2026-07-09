#include "TrainService.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

TrainService::TrainService(QSqlDatabase &db) : m_db(db) {}

/**
 * @brief 车次模糊查询 (利用 JOIN 动态抓取站名，完全对齐新 Train 结构体)
 */
QVector<Train> TrainService::searchTrains(const QString &trainNo, const QString &from, const QDate &date) {
    QString cleanTrainNo = trainNo.toUpper().trimmed(); 
    QString cleanFrom    = from.trimmed();              

    QVector<Train> result;
    if (!m_db.isOpen()) return result;

    QSqlQuery query(m_db);
    //多表联查：通过 from_station_id 和 to_station_id 关联 stations 表获取真实站名
    query.prepare("SELECT t.*, s1.name AS from_name, s2.name AS to_name "
                  "FROM trains t "
                  "JOIN stations s1 ON t.from_station_id = s1.id "
                  "JOIN stations s2 ON t.to_station_id = s2.id "
                  "WHERE (t.train_no LIKE :trainNo OR :trainNoEmpty) AND "
                  "(s1.name = :from OR :fromEmpty) AND "
                  "(t.travel_date = :date OR :dateEmpty)");
    
    query.bindValue(":trainNo", "%" + cleanTrainNo + "%");
    query.bindValue(":trainNoEmpty", cleanTrainNo.isEmpty());
    query.bindValue(":from", cleanFrom);
    query.bindValue(":fromEmpty", cleanFrom.isEmpty());
    query.bindValue(":date", date.isValid() ? date.toString("yyyy-MM-dd") : "");
    query.bindValue(":dateEmpty", !date.isValid());

    if (query.exec()) {
        while (query.next()) {
            Train t;
            //严格映射到全新定义的 Train 结构体字段
            t.id            = query.value("id").toInt();
            t.trainNo       = query.value("train_no").toString(); 
            t.fromStationId = query.value("from_station_id").toInt();
            t.toStationId   = query.value("to_station_id").toInt();
            t.fromStation   = query.value("from_name").toString();  
            t.toStation     = query.value("to_name").toString();     
            t.departTime    = query.value("depart_time").toString();
            t.arriveTime    = query.value("arrive_time").toString(); 
            t.travelDate    = query.value("travel_date").toDate();   
            t.basePrice     = query.value("base_price").toDouble();  
            t.status        = query.value("status").toString();
            
            result.append(t);
        }
    }
    return result;
}

/**
 * @brief 余票精确查询 (多表联合，车次 + 席别组合出 TrainTicketInfo)
 */
QVector<TrainTicketInfo> TrainService::queryRemainingTickets(const QString &from, const QString &to, const QDate &date) {
    QString cleanFrom = from.trimmed();
    QString cleanTo   = to.trimmed();

    QVector<TrainTicketInfo> result;
    if (cleanFrom.isEmpty() || cleanTo.isEmpty() || cleanFrom == cleanTo) {
        return result;
    }

    QSqlQuery query(m_db);
    //将车次主表与席别库存表 JOIN，拉出前台表格需要的“车次+席别”大宽表
    query.prepare("SELECT t.id AS t_id, t.train_no, s1.name AS from_name, s2.name AS to_name, "
                  "t.travel_date, t.depart_time, t.arrive_time, i.seat_type, i.price, i.total_seats, i.remaining_seats "
                  "FROM trains t "
                  "JOIN seat_inventory i ON t.id = i.train_id "
                  "JOIN stations s1 ON t.from_station_id = s1.id "
                  "JOIN stations s2 ON t.to_station_id = s2.id "
                  "WHERE s1.name = :from AND s2.name = :to AND t.travel_date = :date AND t.status = 'running'");
    
    query.bindValue(":from", cleanFrom);
    query.bindValue(":to", cleanTo);
    query.bindValue(":date", date.toString("yyyy-MM-dd"));

    if (query.exec()) {
        while (query.next()) {
            TrainTicketInfo info;
            // 严格映射到全新定义的 TrainTicketInfo 结构体字段
            info.trainId        = query.value("t_id").toInt();        
            info.trainNo        = query.value("train_no").toString();
            info.fromStation    = query.value("from_name").toString();
            info.toStation      = query.value("to_name").toString();
            info.travelDate     = query.value("travel_date").toDate();
            info.departTime     = query.value("depart_time").toString();
            info.arriveTime     = query.value("arrive_time").toString();
            info.seatType       = query.value("seat_type").toString();
            info.price          = query.value("price").toDouble();
            info.totalSeats     = query.value("total_seats").toInt();
            info.remainingSeats = query.value("remaining_seats").toInt();
            
            result.append(info);
        }
    }
    return result;
}

// =========================================================================
// 3. 其余接口对齐新规范修改
// =========================================================================
bool TrainService::addTrain(const Train &t, QString *err) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO trains (train_no, from_station_id, to_station_id, depart_time, arrive_time, travel_date, base_price, status) "
                  "VALUES (:no, :fromId, :toId, :dep, :arr, :date, :price, :status)");
    query.bindValue(":no", t.trainNo.toUpper().trimmed());
    query.bindValue(":fromId", t.fromStationId);
    query.bindValue(":toId", t.toStationId);
    query.bindValue(":dep", t.departTime);
    query.bindValue(":arr", t.arriveTime);
    query.bindValue(":date", t.travelDate.toString("yyyy-MM-dd"));
    query.bindValue(":price", t.basePrice);
    query.bindValue(":status", t.status.isEmpty() ? "running" : t.status);
    if (!query.exec() && err) *err = query.lastError().text();
    return query.isActive();
}

bool TrainService::updateTrain(const Train &t, QString *err) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE trains SET train_no=:no, from_station_id=:fromId, to_station_id=:toId, "
                  "depart_time=:dep, arrive_time=:arr, travel_date=:date, base_price=:price, status=:status WHERE id=:id");
    query.bindValue(":no", t.trainNo.toUpper().trimmed());
    query.bindValue(":fromId", t.fromStationId);
    query.bindValue(":toId", t.toStationId);
    query.bindValue(":dep", t.departTime);
    query.bindValue(":arr", t.arriveTime);
    query.bindValue(":date", t.travelDate.toString("yyyy-MM-dd"));
    query.bindValue(":price", t.basePrice);
    query.bindValue(":status", t.status);
    query.bindValue(":id", t.id);
    if (!query.exec() && err) *err = query.lastError().text();
    return query.isActive();
}

bool TrainService::deleteTrain(int trainId, QString *err) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM trains WHERE id = :id");
    query.bindValue(":id", trainId);
    if (!query.exec() && err) *err = query.lastError().text();
    return query.isActive();
}

bool TrainService::setTrainStatus(int trainId, const QString &status, QString *err) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE trains SET status = :status WHERE id = :id");
    query.bindValue(":status", status.trimmed());
    query.bindValue(":id", trainId);
    if (!query.exec() && err) *err = query.lastError().text();
    return query.isActive();
}

bool TrainService::configureSeats(int trainId, const QString &seatType, int total, QString *err) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE seat_inventory SET total_seats = :total, remaining_seats = :total WHERE train_id = :id AND seat_type = :type");
    query.bindValue(":total", total);
    query.bindValue(":id", trainId);
    query.bindValue(":type", seatType.trimmed());
    if (!query.exec() && err) *err = query.lastError().text();
    return query.isActive();
}

bool TrainService::isTrainBookable(int trainId) {
    QSqlQuery query(m_db);
    query.prepare("SELECT status FROM trains WHERE id = :id");
    query.bindValue(":id", trainId);
    if (query.exec() && query.next()) {
        return query.value("status").toString() == "running";
    }
    return false;
}

int TrainService::getRemainingSeats(int trainId, const QString &seatType) {
    QSqlQuery query(m_db);
    query.prepare("SELECT remaining_seats FROM seat_inventory WHERE train_id = :id AND seat_type = :type");
    query.bindValue(":id", trainId);
    query.bindValue(":type", seatType.trimmed());
    if (query.exec() && query.next()) {
        return query.value("remaining_seats").toInt();
    }
    return 0;
} 

std::optional<Train> TrainService::getTrainById(int trainId) {
    QSqlQuery query(m_db);
    query.prepare("SELECT t.*, s1.name AS from_name, s2.name AS to_name "
                  "FROM trains t "
                  "JOIN stations s1 ON t.from_station_id = s1.id "
                  "JOIN stations s2 ON t.to_station_id = s2.id WHERE t.id = :id");
    query.bindValue(":id", trainId);
    if (query.exec() && query.next()) {
        Train t;
        t.id            = query.value("id").toInt();
        t.trainNo       = query.value("train_no").toString();
        t.fromStationId = query.value("from_station_id").toInt();
        t.toStationId   = query.value("to_station_id").toInt();
        t.fromStation   = query.value("from_name").toString();
        t.toStation     = query.value("to_name").toString();
        t.departTime    = query.value("depart_time").toString();
        t.arriveTime    = query.value("arrive_time").toString();
        t.travelDate    = query.value("travel_date").toDate();
        t.basePrice     = query.value("base_price").toDouble();
        t.status        = query.value("status").toString();
        return t;
    }
    return std::nullopt;
}