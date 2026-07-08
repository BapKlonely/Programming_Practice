#include "datastore.h"

#include <QDateTime>
#include <QDir>
#include <QRandomGenerator>
#include <QSqlError>
#include <QSqlQuery>
#include <QTime>

static QString sqlDate(const QDate &date)
{
    return date.toString(Qt::ISODate);
}

static QDate fromSqlDate(const QVariant &value)
{
    return QDate::fromString(value.toString(), Qt::ISODate);
}

static void bindTrain(QSqlQuery &query, const Train &train)
{
    query.bindValue(":number", train.number);
    query.bindValue(":from_city", train.from);
    query.bindValue(":to_city", train.to);
    query.bindValue(":depart_date", sqlDate(train.date));
    query.bindValue(":depart_time", train.departTime);
    query.bindValue(":arrive_date", sqlDate(train.arriveDate));
    query.bindValue(":arrive_time", train.arriveTime);
    query.bindValue(":seat_type", train.seatType);
    query.bindValue(":price", train.price);
    query.bindValue(":total_seats", train.totalSeats);
    query.bindValue(":left_seats", train.leftSeats);
}

DataStore::DataStore(const QString &basePath)
    : m_basePath(basePath)
{
}

bool DataStore::load()
{
    if (!openDatabase() || !createTables()) {
        return false;
    }

    const bool ok = loadTrains() && loadUsers() && loadOrders();
    seedIfEmpty();

    for (auto &train : m_trains) {
        if (!train.arriveDate.isValid()) {
            train.arriveDate = train.date;
            if (QTime::fromString(train.arriveTime, "hh:mm") <= QTime::fromString(train.departTime, "hh:mm")) {
                train.arriveDate = train.arriveDate.addDays(1);
            }
        }
    }

    return ok && saveAll();
}

bool DataStore::saveAll() const
{
    return saveTrains() && saveUsers() && saveOrders();
}

QVector<Train> DataStore::trains() const { return m_trains; }
QVector<User> DataStore::users() const { return m_users; }
QVector<Order> DataStore::orders() const { return m_orders; }

bool DataStore::registerUser(const User &user, QString *error)
{
    if (user.username.trimmed().isEmpty() || user.password.isEmpty()) {
        if (error) *error = "用户名和密码不能为空";
        return false;
    }
    if (findUser(user.username)) {
        if (error) *error = "用户名已存在";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO users(username, password, name, phone, admin)
        VALUES(:username, :password, :name, :phone, :admin)
    )");
    query.bindValue(":username", user.username);
    query.bindValue(":password", user.password);
    query.bindValue(":name", user.name);
    query.bindValue(":phone", user.phone);
    query.bindValue(":admin", user.admin ? 1 : 0);
    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }

    m_users.push_back(user);
    return true;
}

const User *DataStore::findUser(const QString &username) const
{
    for (const auto &user : m_users) {
        if (user.username == username) {
            return &user;
        }
    }
    return nullptr;
}

bool DataStore::upsertTrain(const Train &train, QString *error)
{
    if (train.number.trimmed().isEmpty() || train.from.trimmed().isEmpty() || train.to.trimmed().isEmpty()) {
        if (error) *error = "车次、出发地和目的地不能为空";
        return false;
    }
    if (!train.date.isValid() || !train.arriveDate.isValid()) {
        if (error) *error = "日期无效";
        return false;
    }
    if (QDateTime(train.arriveDate, QTime::fromString(train.arriveTime, "hh:mm")) <
        QDateTime(train.date, QTime::fromString(train.departTime, "hh:mm"))) {
        if (error) *error = "到达时间不能早于出发时间";
        return false;
    }
    if (train.totalSeats < 0 || train.leftSeats < 0 || train.leftSeats > train.totalSeats || train.price < 0) {
        if (error) *error = "票价或座位数无效";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO trains(number, from_city, to_city, depart_date, depart_time, arrive_date, arrive_time,
                           seat_type, price, total_seats, left_seats)
        VALUES(:number, :from_city, :to_city, :depart_date, :depart_time, :arrive_date, :arrive_time,
               :seat_type, :price, :total_seats, :left_seats)
        ON CONFLICT(number) DO UPDATE SET
            from_city = excluded.from_city,
            to_city = excluded.to_city,
            depart_date = excluded.depart_date,
            depart_time = excluded.depart_time,
            arrive_date = excluded.arrive_date,
            arrive_time = excluded.arrive_time,
            seat_type = excluded.seat_type,
            price = excluded.price,
            total_seats = excluded.total_seats,
            left_seats = excluded.left_seats
    )");
    bindTrain(query, train);
    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }

    for (auto &current : m_trains) {
        if (current.number == train.number) {
            current = train;
            return true;
        }
    }
    m_trains.push_back(train);
    return true;
}

bool DataStore::removeTrain(const QString &number, QString *error)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM trains WHERE number = :number");
    query.bindValue(":number", number);
    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        if (error) *error = "未找到该车次";
        return false;
    }

    for (int i = 0; i < m_trains.size(); ++i) {
        if (m_trains[i].number == number) {
            m_trains.removeAt(i);
            break;
        }
    }
    return true;
}

Train *DataStore::findTrain(const QString &number)
{
    for (auto &train : m_trains) {
        if (train.number == number) {
            return &train;
        }
    }
    return nullptr;
}

const Train *DataStore::findTrain(const QString &number) const
{
    for (const auto &train : m_trains) {
        if (train.number == number) {
            return &train;
        }
    }
    return nullptr;
}

bool DataStore::createOrder(const QString &username,
                            const Train &train,
                            const QString &passengerName,
                            const QString &passengerId,
                            int count,
                            Order *created,
                            QString *error)
{
    Train *storedTrain = findTrain(train.number);
    if (!storedTrain) {
        if (error) *error = "车次不存在";
        return false;
    }
    if (passengerName.trimmed().isEmpty() || passengerId.trimmed().isEmpty()) {
        if (error) *error = "乘车人姓名和证件号不能为空";
        return false;
    }
    if (count <= 0 || count > storedTrain->leftSeats) {
        if (error) *error = "余票不足";
        return false;
    }

    const QString orderId = QString("O%1%2")
        .arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"))
        .arg(QRandomGenerator::global()->bounded(1000, 9999));

    if (!m_db.transaction()) {
        if (error) *error = m_db.lastError().text();
        return false;
    }

    QSqlQuery updateTrain(m_db);
    updateTrain.prepare(R"(
        UPDATE trains
        SET left_seats = left_seats - :count
        WHERE number = :number AND left_seats >= :count
    )");
    updateTrain.bindValue(":count", count);
    updateTrain.bindValue(":number", storedTrain->number);
    if (!updateTrain.exec() || updateTrain.numRowsAffected() != 1) {
        m_db.rollback();
        if (error) *error = "余票不足";
        return false;
    }

    Order order;
    order.id = orderId;
    order.username = username;
    order.trainNumber = storedTrain->number;
    order.from = storedTrain->from;
    order.to = storedTrain->to;
    order.date = storedTrain->date;
    order.departTime = storedTrain->departTime;
    order.passengerName = passengerName;
    order.passengerId = passengerId;
    order.count = count;
    order.amount = storedTrain->price * count;
    order.status = "已出票";
    order.createdAt = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    QSqlQuery insertOrder(m_db);
    insertOrder.prepare(R"(
        INSERT INTO orders(id, username, train_number, from_city, to_city, depart_date, depart_time,
                           passenger_name, passenger_id, count, amount, status, created_at)
        VALUES(:id, :username, :train_number, :from_city, :to_city, :depart_date, :depart_time,
               :passenger_name, :passenger_id, :count, :amount, :status, :created_at)
    )");
    insertOrder.bindValue(":id", order.id);
    insertOrder.bindValue(":username", order.username);
    insertOrder.bindValue(":train_number", order.trainNumber);
    insertOrder.bindValue(":from_city", order.from);
    insertOrder.bindValue(":to_city", order.to);
    insertOrder.bindValue(":depart_date", sqlDate(order.date));
    insertOrder.bindValue(":depart_time", order.departTime);
    insertOrder.bindValue(":passenger_name", order.passengerName);
    insertOrder.bindValue(":passenger_id", order.passengerId);
    insertOrder.bindValue(":count", order.count);
    insertOrder.bindValue(":amount", order.amount);
    insertOrder.bindValue(":status", order.status);
    insertOrder.bindValue(":created_at", order.createdAt);
    if (!insertOrder.exec()) {
        m_db.rollback();
        if (error) *error = insertOrder.lastError().text();
        return false;
    }

    if (!m_db.commit()) {
        if (error) *error = m_db.lastError().text();
        return false;
    }

    storedTrain->leftSeats -= count;
    m_orders.push_back(order);
    if (created) *created = order;
    return true;
}

bool DataStore::refundOrder(const QString &orderId, QString *error)
{
    for (auto &order : m_orders) {
        if (order.id == orderId) {
            if (order.status == "已退票") {
                if (error) *error = "订单已退票";
                return false;
            }

            if (!m_db.transaction()) {
                if (error) *error = m_db.lastError().text();
                return false;
            }

            QSqlQuery updateOrder(m_db);
            updateOrder.prepare("UPDATE orders SET status = :status WHERE id = :id AND status <> :status");
            updateOrder.bindValue(":status", "已退票");
            updateOrder.bindValue(":id", orderId);
            if (!updateOrder.exec() || updateOrder.numRowsAffected() != 1) {
                m_db.rollback();
                if (error) *error = "订单已退票或不存在";
                return false;
            }

            QSqlQuery updateTrain(m_db);
            updateTrain.prepare(R"(
                UPDATE trains
                SET left_seats = CASE
                    WHEN left_seats + :count > total_seats THEN total_seats
                    ELSE left_seats + :count
                END
                WHERE number = :number
            )");
            updateTrain.bindValue(":count", order.count);
            updateTrain.bindValue(":number", order.trainNumber);
            if (!updateTrain.exec()) {
                m_db.rollback();
                if (error) *error = updateTrain.lastError().text();
                return false;
            }

            if (!m_db.commit()) {
                if (error) *error = m_db.lastError().text();
                return false;
            }

            Train *train = findTrain(order.trainNumber);
            if (train) {
                train->leftSeats = qMin(train->totalSeats, train->leftSeats + order.count);
            }
            order.status = "已退票";
            return true;
        }
    }

    if (error) *error = "未找到订单";
    return false;
}

bool DataStore::openDatabase()
{
    QDir().mkpath(m_basePath);
    const QString connectionName = QString("train_ticket_%1").arg(reinterpret_cast<quintptr>(this));
    m_db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    m_db.setDatabaseName(databaseFile());
    return m_db.open();
}

bool DataStore::createTables()
{
    QSqlQuery query(m_db);
    return query.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            username TEXT PRIMARY KEY,
            password TEXT NOT NULL,
            name TEXT,
            phone TEXT,
            admin INTEGER NOT NULL DEFAULT 0
        )
    )") && query.exec(R"(
        CREATE TABLE IF NOT EXISTS trains (
            number TEXT PRIMARY KEY,
            from_city TEXT NOT NULL,
            to_city TEXT NOT NULL,
            depart_date TEXT NOT NULL,
            depart_time TEXT NOT NULL,
            arrive_date TEXT NOT NULL,
            arrive_time TEXT NOT NULL,
            seat_type TEXT,
            price REAL NOT NULL DEFAULT 0,
            total_seats INTEGER NOT NULL DEFAULT 0,
            left_seats INTEGER NOT NULL DEFAULT 0
        )
    )") && query.exec(R"(
        CREATE TABLE IF NOT EXISTS orders (
            id TEXT PRIMARY KEY,
            username TEXT NOT NULL,
            train_number TEXT NOT NULL,
            from_city TEXT NOT NULL,
            to_city TEXT NOT NULL,
            depart_date TEXT NOT NULL,
            depart_time TEXT NOT NULL,
            passenger_name TEXT NOT NULL,
            passenger_id TEXT NOT NULL,
            count INTEGER NOT NULL,
            amount REAL NOT NULL,
            status TEXT NOT NULL,
            created_at TEXT NOT NULL
        )
    )");
}

bool DataStore::loadTrains()
{
    QSqlQuery query(m_db);
    if (!query.exec(R"(
        SELECT number, from_city, to_city, depart_date, depart_time, arrive_date, arrive_time,
               seat_type, price, total_seats, left_seats
        FROM trains
        ORDER BY depart_date, depart_time, number
    )")) {
        return false;
    }

    m_trains.clear();
    while (query.next()) {
        Train train;
        train.number = query.value(0).toString();
        train.from = query.value(1).toString();
        train.to = query.value(2).toString();
        train.date = fromSqlDate(query.value(3));
        train.departTime = query.value(4).toString();
        train.arriveDate = fromSqlDate(query.value(5));
        train.arriveTime = query.value(6).toString();
        train.seatType = query.value(7).toString();
        train.price = query.value(8).toDouble();
        train.totalSeats = query.value(9).toInt();
        train.leftSeats = query.value(10).toInt();
        m_trains.push_back(train);
    }
    return true;
}

bool DataStore::loadUsers()
{
    QSqlQuery query(m_db);
    if (!query.exec("SELECT username, password, name, phone, admin FROM users ORDER BY username")) {
        return false;
    }

    m_users.clear();
    while (query.next()) {
        User user;
        user.username = query.value(0).toString();
        user.password = query.value(1).toString();
        user.name = query.value(2).toString();
        user.phone = query.value(3).toString();
        user.admin = query.value(4).toInt() != 0;
        m_users.push_back(user);
    }
    return true;
}

bool DataStore::loadOrders()
{
    QSqlQuery query(m_db);
    if (!query.exec(R"(
        SELECT id, username, train_number, from_city, to_city, depart_date, depart_time,
               passenger_name, passenger_id, count, amount, status, created_at
        FROM orders
        ORDER BY created_at DESC
    )")) {
        return false;
    }

    m_orders.clear();
    while (query.next()) {
        Order order;
        order.id = query.value(0).toString();
        order.username = query.value(1).toString();
        order.trainNumber = query.value(2).toString();
        order.from = query.value(3).toString();
        order.to = query.value(4).toString();
        order.date = fromSqlDate(query.value(5));
        order.departTime = query.value(6).toString();
        order.passengerName = query.value(7).toString();
        order.passengerId = query.value(8).toString();
        order.count = query.value(9).toInt();
        order.amount = query.value(10).toDouble();
        order.status = query.value(11).toString();
        order.createdAt = query.value(12).toString();
        m_orders.push_back(order);
    }
    return true;
}

bool DataStore::saveTrains() const
{
    QSqlQuery clear(m_db);
    if (!clear.exec("DELETE FROM trains")) return false;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO trains(number, from_city, to_city, depart_date, depart_time, arrive_date, arrive_time,
                           seat_type, price, total_seats, left_seats)
        VALUES(:number, :from_city, :to_city, :depart_date, :depart_time, :arrive_date, :arrive_time,
               :seat_type, :price, :total_seats, :left_seats)
    )");
    for (const auto &train : m_trains) {
        bindTrain(query, train);
        if (!query.exec()) return false;
    }
    return true;
}

bool DataStore::saveUsers() const
{
    QSqlQuery clear(m_db);
    if (!clear.exec("DELETE FROM users")) return false;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO users(username, password, name, phone, admin)
        VALUES(:username, :password, :name, :phone, :admin)
    )");
    for (const auto &user : m_users) {
        query.bindValue(":username", user.username);
        query.bindValue(":password", user.password);
        query.bindValue(":name", user.name);
        query.bindValue(":phone", user.phone);
        query.bindValue(":admin", user.admin ? 1 : 0);
        if (!query.exec()) return false;
    }
    return true;
}

bool DataStore::saveOrders() const
{
    QSqlQuery clear(m_db);
    if (!clear.exec("DELETE FROM orders")) return false;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO orders(id, username, train_number, from_city, to_city, depart_date, depart_time,
                           passenger_name, passenger_id, count, amount, status, created_at)
        VALUES(:id, :username, :train_number, :from_city, :to_city, :depart_date, :depart_time,
               :passenger_name, :passenger_id, :count, :amount, :status, :created_at)
    )");
    for (const auto &order : m_orders) {
        query.bindValue(":id", order.id);
        query.bindValue(":username", order.username);
        query.bindValue(":train_number", order.trainNumber);
        query.bindValue(":from_city", order.from);
        query.bindValue(":to_city", order.to);
        query.bindValue(":depart_date", sqlDate(order.date));
        query.bindValue(":depart_time", order.departTime);
        query.bindValue(":passenger_name", order.passengerName);
        query.bindValue(":passenger_id", order.passengerId);
        query.bindValue(":count", order.count);
        query.bindValue(":amount", order.amount);
        query.bindValue(":status", order.status);
        query.bindValue(":created_at", order.createdAt);
        if (!query.exec()) return false;
    }
    return true;
}

void DataStore::seedIfEmpty()
{
    if (!findUser("admin")) {
        registerUser({"admin", "admin123", "系统管理员", "00000000000", true}, nullptr);
    }
    if (!m_trains.isEmpty()) return;

    const QDate today = QDate::currentDate();
    upsertTrain({"G101", "北京", "上海", today.addDays(1), "08:00", "12:36", "二等座", 553.0, 120, 120, today.addDays(1)}, nullptr);
    upsertTrain({"G305", "上海", "广州", today.addDays(1), "09:20", "16:10", "一等座", 988.0, 80, 80, today.addDays(1)}, nullptr);
    upsertTrain({"D2201", "成都", "武汉", today.addDays(2), "07:15", "15:42", "二等座", 356.0, 100, 100, today.addDays(2)}, nullptr);
    upsertTrain({"K88", "广州", "北京", today.addDays(2), "18:45", "10:30", "硬卧", 426.5, 160, 160, today.addDays(3)}, nullptr);
    upsertTrain({"Z19", "北京", "西安", today.addDays(3), "20:40", "08:31", "软卧", 415.0, 60, 60, today.addDays(4)}, nullptr);
}

QString DataStore::databaseFile() const
{
    return QDir(m_basePath).filePath("tickets.db");
}
