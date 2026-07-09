#include "services/TrainService.h"

#include "core/database.h"

#include <QSqlError>
#include <QSqlQuery>

TrainService::TrainService(Database &db)
    : m_db(db)
{
}

/**
 * @brief 车次模糊查询（JOIN stations 获取站名，映射到 Train 结构体）
 */
QVector<Train> TrainService::searchTrains(const QString &trainNo, const QString &from, const QDate &date)
{
    const QString cleanTrainNo = trainNo.toUpper().trimmed();
    const QString cleanFrom = from.trimmed();

    QVector<Train> result;
    if (!m_db.isOpen()) {
        return result;
    }

    QSqlQuery query(m_db.connection());
    query.prepare(QStringLiteral(
        "SELECT t.*, s1.name AS from_name, s2.name AS to_name "
        "FROM trains t "
        "JOIN stations s1 ON t.from_station_id = s1.id "
        "JOIN stations s2 ON t.to_station_id = s2.id "
        "WHERE (t.train_no LIKE :trainNo OR :trainNoEmpty) AND "
        "(s1.name = :from OR :fromEmpty) AND "
        "(t.travel_date = :date OR :dateEmpty)"));

    query.bindValue(QStringLiteral(":trainNo"), QStringLiteral("%%1%").arg(cleanTrainNo));
    query.bindValue(QStringLiteral(":trainNoEmpty"), cleanTrainNo.isEmpty());
    query.bindValue(QStringLiteral(":from"), cleanFrom);
    query.bindValue(QStringLiteral(":fromEmpty"), cleanFrom.isEmpty());
    query.bindValue(QStringLiteral(":date"), date.isValid() ? date.toString(QStringLiteral("yyyy-MM-dd")) : QString());
    query.bindValue(QStringLiteral(":dateEmpty"), !date.isValid());

    if (query.exec()) {
        while (query.next()) {
            Train t;
            t.id = query.value(QStringLiteral("id")).toInt();
            t.trainNo = query.value(QStringLiteral("train_no")).toString();
            t.fromStationId = query.value(QStringLiteral("from_station_id")).toInt();
            t.toStationId = query.value(QStringLiteral("to_station_id")).toInt();
            t.fromStation = query.value(QStringLiteral("from_name")).toString();
            t.toStation = query.value(QStringLiteral("to_name")).toString();
            t.departTime = query.value(QStringLiteral("depart_time")).toString();
            t.arriveTime = query.value(QStringLiteral("arrive_time")).toString();
            t.travelDate = query.value(QStringLiteral("travel_date")).toDate();
            t.basePrice = query.value(QStringLiteral("base_price")).toDouble();
            t.status = query.value(QStringLiteral("status")).toString();
            result.append(t);
        }
    }
    return result;
}

/**
 * @brief 余票精确查询（车次 + 席别，映射到 TrainTicketInfo）
 *
 * seat_inventory 使用 total / sold 字段，余票 = total - sold；
 * 票价展示使用 trains.base_price（与 TicketService::calculatePrice 基准一致）。
 */
QVector<TrainTicketInfo> TrainService::queryRemainingTickets(const QString &from,
                                                             const QString &to,
                                                             const QDate &date)
{
    const QString cleanFrom = from.trimmed();
    const QString cleanTo = to.trimmed();

    QVector<TrainTicketInfo> result;
    if (cleanFrom.isEmpty() || cleanTo.isEmpty() || cleanFrom == cleanTo) {
        return result;
    }

    QSqlQuery query(m_db.connection());
    query.prepare(QStringLiteral(
        "SELECT t.id AS t_id, t.train_no, s1.name AS from_name, s2.name AS to_name, "
        "t.travel_date, t.depart_time, t.arrive_time, t.base_price, "
        "i.seat_type, i.total AS total_seats, (i.total - i.sold) AS remaining_seats "
        "FROM trains t "
        "JOIN seat_inventory i ON t.id = i.train_id "
        "JOIN stations s1 ON t.from_station_id = s1.id "
        "JOIN stations s2 ON t.to_station_id = s2.id "
        "WHERE s1.name = :from AND s2.name = :to AND t.travel_date = :date AND t.status = 'running'"));

    query.bindValue(QStringLiteral(":from"), cleanFrom);
    query.bindValue(QStringLiteral(":to"), cleanTo);
    query.bindValue(QStringLiteral(":date"), date.toString(QStringLiteral("yyyy-MM-dd")));

    if (query.exec()) {
        while (query.next()) {
            TrainTicketInfo info;
            info.trainId = query.value(QStringLiteral("t_id")).toInt();
            info.trainNo = query.value(QStringLiteral("train_no")).toString();
            info.fromStation = query.value(QStringLiteral("from_name")).toString();
            info.toStation = query.value(QStringLiteral("to_name")).toString();
            info.travelDate = query.value(QStringLiteral("travel_date")).toDate();
            info.departTime = query.value(QStringLiteral("depart_time")).toString();
            info.arriveTime = query.value(QStringLiteral("arrive_time")).toString();
            info.seatType = query.value(QStringLiteral("seat_type")).toString();
            info.price = query.value(QStringLiteral("base_price")).toDouble();
            info.totalSeats = query.value(QStringLiteral("total_seats")).toInt();
            info.remainingSeats = query.value(QStringLiteral("remaining_seats")).toInt();
            result.append(info);
        }
    }
    return result;
}

bool TrainService::addTrain(const Train &t, QString *err)
{
    QSqlQuery query(m_db.connection());
    query.prepare(QStringLiteral(
        "INSERT INTO trains (train_no, from_station_id, to_station_id, depart_time, arrive_time, travel_date, base_price, status) "
        "VALUES (:no, :fromId, :toId, :dep, :arr, :date, :price, :status)"));
    query.bindValue(QStringLiteral(":no"), t.trainNo.toUpper().trimmed());
    query.bindValue(QStringLiteral(":fromId"), t.fromStationId);
    query.bindValue(QStringLiteral(":toId"), t.toStationId);
    query.bindValue(QStringLiteral(":dep"), t.departTime);
    query.bindValue(QStringLiteral(":arr"), t.arriveTime);
    query.bindValue(QStringLiteral(":date"), t.travelDate.toString(QStringLiteral("yyyy-MM-dd")));
    query.bindValue(QStringLiteral(":price"), t.basePrice);
    query.bindValue(QStringLiteral(":status"), t.status.isEmpty() ? QStringLiteral("running") : t.status);

    const bool ok = query.exec();
    if (!ok && err) {
        *err = query.lastError().text();
    }
    return ok;
}

bool TrainService::updateTrain(const Train &t, QString *err)
{
    QSqlQuery query(m_db.connection());
    query.prepare(QStringLiteral(
        "UPDATE trains SET train_no=:no, from_station_id=:fromId, to_station_id=:toId, "
        "depart_time=:dep, arrive_time=:arr, travel_date=:date, base_price=:price, status=:status "
        "WHERE id=:id"));
    query.bindValue(QStringLiteral(":no"), t.trainNo.toUpper().trimmed());
    query.bindValue(QStringLiteral(":fromId"), t.fromStationId);
    query.bindValue(QStringLiteral(":toId"), t.toStationId);
    query.bindValue(QStringLiteral(":dep"), t.departTime);
    query.bindValue(QStringLiteral(":arr"), t.arriveTime);
    query.bindValue(QStringLiteral(":date"), t.travelDate.toString(QStringLiteral("yyyy-MM-dd")));
    query.bindValue(QStringLiteral(":price"), t.basePrice);
    query.bindValue(QStringLiteral(":status"), t.status);
    query.bindValue(QStringLiteral(":id"), t.id);

    const bool ok = query.exec();
    if (!ok && err) {
        *err = query.lastError().text();
    }
    return ok;
}

bool TrainService::deleteTrain(int trainId, QString *err)
{
    QSqlQuery query(m_db.connection());
    query.prepare(QStringLiteral("DELETE FROM trains WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), trainId);

    const bool ok = query.exec();
    if (!ok && err) {
        *err = query.lastError().text();
    }
    return ok;
}

bool TrainService::setTrainStatus(int trainId, const QString &status, QString *err)
{
    QSqlQuery query(m_db.connection());
    query.prepare(QStringLiteral("UPDATE trains SET status = :status WHERE id = :id"));
    query.bindValue(QStringLiteral(":status"), status.trimmed());
    query.bindValue(QStringLiteral(":id"), trainId);

    const bool ok = query.exec();
    if (!ok && err) {
        *err = query.lastError().text();
    }
    return ok;
}

bool TrainService::configureSeats(int trainId, const QString &seatType, int total, QString *err)
{
    QSqlQuery query(m_db.connection());
    // 与成员 D 语义一致：配置后余票等于总量，即 sold 归零
    query.prepare(QStringLiteral(
        "UPDATE seat_inventory SET total = :total, sold = 0 "
        "WHERE train_id = :id AND seat_type = :type"));
    query.bindValue(QStringLiteral(":total"), total);
    query.bindValue(QStringLiteral(":id"), trainId);
    query.bindValue(QStringLiteral(":type"), seatType.trimmed());

    const bool ok = query.exec();
    if (!ok && err) {
        *err = query.lastError().text();
    }
    return ok;
}

bool TrainService::isTrainBookable(int trainId)
{
    QSqlQuery query(m_db.connection());
    query.prepare(QStringLiteral("SELECT status FROM trains WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), trainId);
    if (query.exec() && query.next()) {
        return query.value(0).toString() == QStringLiteral("running");
    }
    return false;
}

int TrainService::getRemainingSeats(int trainId, const QString &seatType)
{
    QSqlQuery query(m_db.connection());
    query.prepare(QStringLiteral(
        "SELECT total - sold FROM seat_inventory WHERE train_id = :id AND seat_type = :type"));
    query.bindValue(QStringLiteral(":id"), trainId);
    query.bindValue(QStringLiteral(":type"), seatType.trimmed());
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

std::optional<Train> TrainService::getTrainById(int trainId)
{
    QSqlQuery query(m_db.connection());
    query.prepare(QStringLiteral(
        "SELECT t.*, s1.name AS from_name, s2.name AS to_name "
        "FROM trains t "
        "JOIN stations s1 ON t.from_station_id = s1.id "
        "JOIN stations s2 ON t.to_station_id = s2.id "
        "WHERE t.id = :id"));
    query.bindValue(QStringLiteral(":id"), trainId);
    if (query.exec() && query.next()) {
        Train t;
        t.id = query.value(QStringLiteral("id")).toInt();
        t.trainNo = query.value(QStringLiteral("train_no")).toString();
        t.fromStationId = query.value(QStringLiteral("from_station_id")).toInt();
        t.toStationId = query.value(QStringLiteral("to_station_id")).toInt();
        t.fromStation = query.value(QStringLiteral("from_name")).toString();
        t.toStation = query.value(QStringLiteral("to_name")).toString();
        t.departTime = query.value(QStringLiteral("depart_time")).toString();
        t.arriveTime = query.value(QStringLiteral("arrive_time")).toString();
        t.travelDate = query.value(QStringLiteral("travel_date")).toDate();
        t.basePrice = query.value(QStringLiteral("base_price")).toDouble();
        t.status = query.value(QStringLiteral("status")).toString();
        return t;
    }
    return std::nullopt;
}
