#include "TrainService.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

// =========================================================================
// 1. 构造函数
// =========================================================================
TrainService::TrainService(QSqlDatabase &db) : m_db(db) {}

// =========================================================================
// 2. 核心业务：查询接口（严格对齐 train.h 变量名，带强力容错）
// =========================================================================

/**
 * @brief 车次模糊查询
 */
QVector<Train> TrainService::searchTrains(const QString &trainNo, const QString &from, const QDate &date) {
    // 🛠️ 落实输入容错防线
    QString cleanTrainNo = trainNo.toUpper().trimmed(); 
    QString cleanFrom    = from.trimmed();              

    QVector<Train> result;
    if (!m_db.isOpen()) return result;

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM trains WHERE "
                  "(train_no LIKE :trainNo OR :trainNoEmpty) AND "
                  "(start_station = :from OR :fromEmpty) AND "
                  "(start_date = :date OR :dateEmpty)");
    
    query.bindValue(":trainNo", "%" + cleanTrainNo + "%");
    query.bindValue(":trainNoEmpty", cleanTrainNo.isEmpty());
    query.bindValue(":from", cleanFrom);
    query.bindValue(":fromEmpty", cleanFrom.isEmpty());
    query.bindValue(":date", date.isValid() ? date.toString("yyyy-MM-dd") : "");
    query.bindValue(":dateEmpty", !date.isValid());

    if (query.exec()) {
        while (query.next()) {
            Train t;
            // 📝 严格对齐 Train 结构体：id, trainNo, startStation, endStation, status
            t.id           = query.value("id").toInt();
            t.trainNo      = query.value("train_no").toString(); 
            t.startStation = query.value("start_station").toString();
            t.endStation   = query.value("destination_station").toString();
            t.status       = query.value("status").toString();
            
            result.append(t);
        }
    }
    return result;
}

/**
 * @brief 余票精确查询
 */
QVector<TrainTicketInfo> TrainService::queryRemainingTickets(const QString &from, const QString &to, const QDate &date) {
    // 🛠️ 落实输入容错防线
    QString cleanFrom = from.trimmed();
    QString cleanTo   = to.trimmed();

    QVector<TrainTicketInfo> result;
    if (cleanFrom.isEmpty() || cleanTo.isEmpty() || cleanFrom == cleanTo) {
        return result;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM trains WHERE start_station = :from AND destination_station = :to AND start_date = :date");
    query.bindValue(":from", cleanFrom);
    query.bindValue(":to", cleanTo);
    query.bindValue(":date", date.toString("yyyy-MM-dd"));

    if (query.exec()) {
        while (query.next()) {
            TrainTicketInfo info;
            // 📝 严格对齐 TrainTicketInfo 结构体：trainNo, fromStation, toStation, date, seatType, availableSeats, status
            info.trainNo        = query.value("train_no").toString();
            info.fromStation    = query.value("start_station").toString();
            info.toStation      = query.value("destination_station").toString();
            info.date           = query.value("start_date").toDate();
            info.seatType       = query.value("seat_type").toString();
            info.availableSeats = query.value("rest_seats").toInt(); 
            info.status         = query.value("status").toString();
            
            result.append(info);
        }
    }
    return result;
}

// =========================================================================
// 3. 其它管理接口桩函数
// =========================================================================
bool TrainService::addTrain(const Train&, QString*) { return true; }
bool TrainService::updateTrain(const Train&, QString*) { return true; }
bool TrainService::deleteTrain(int, QString*) { return true; }
bool TrainService::setTrainStatus(int, const QString&, QString*) { return true; }
bool TrainService::configureSeats(int, const QString&, int, QString*) { return true; }
bool TrainService::isTrainBookable(int) { return true; }
int TrainService::getRemainingSeats(int, const QString&) { return 100; } 
std::optional<Train> TrainService::getTrainById(int) { return std::nullopt; }