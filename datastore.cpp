#include "datastore.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

DataStore::DataStore(const QString &dbName) {
    qDebug() << "========================================";
    qDebug() << "SQLite 关系型数据库启动...";

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbName);

    if (!m_db.open()) {
        qDebug() << "错误：数据库打开失败！" << m_db.lastError().text();
    } else {
        qDebug() << "成功：数据库已连接，路径:" << dbName;
        initTables();
        insertMockData();
    }
    qDebug() << "========================================";
}

DataStore::~DataStore() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

QSqlDatabase DataStore::getDatabase() const {
    return m_db;
}

//核心重构：建立符合全新 train.h 规范的三张关联表
void DataStore::initTables() {
    QSqlQuery query(m_db);
    
    // 1. 创建 stations 表
    query.exec("CREATE TABLE IF NOT EXISTS stations ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "name TEXT UNIQUE"
               ");");

    // 2. 创建 trains 主表
    query.exec("CREATE TABLE IF NOT EXISTS trains ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "train_no TEXT, "
               "from_station_id INTEGER, "
               "to_station_id INTEGER, "
               "depart_time TEXT, "
               "arrive_time TEXT, "
               "travel_date TEXT, "
               "base_price REAL, "
               "status TEXT"
               ");");

    // 3. 创建 seat_inventory 库存表
    query.exec("CREATE TABLE IF NOT EXISTS seat_inventory ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "train_id INTEGER, "
               "seat_type TEXT, "
               "price REAL, "
               "total_seats INTEGER, "
               "remaining_seats INTEGER"
               ");");
    
    qDebug() << "关系型数据库三表 [stations, trains, seat_inventory] 准备就绪";
}

// 自动注入满足多表关联的测试假数据
void DataStore::insertMockData() {
    QSqlQuery query(m_db);
    
    query.exec("SELECT COUNT(*) FROM stations");
    if (query.next() && query.value(0).toInt() > 0) return; // 已有数据，跳过

    m_db.transaction(); // 开启事务

    // 1. 插入站点
    query.exec("INSERT INTO stations (name) VALUES ('北京'), ('上海'), ('成都'), ('西安')");

    // 2. 插入车次主表记录 (G114, 北京(1) -> 上海(2))
    query.exec("INSERT INTO trains (train_no, from_station_id, to_station_id, depart_time, arrive_time, travel_date, base_price, status) "
               "VALUES ('G114', 1, 2, '08:00', '12:36', '2026-07-08', 550.0, 'running')");
    int trainId = query.lastInsertId().toInt();

    // 3. 插入该车次的多条席别库存 (满足一行车次对应多条席别记录)
    query.prepare("INSERT INTO seat_inventory (train_id, seat_type, price, total_seats, remaining_seats) VALUES (?, ?, ?, ?, ?)");
    
    // 二等座
    query.addBindValue(trainId); query.addBindValue("二等座"); query.addBindValue(550.0); query.addBindValue(100); query.addBindValue(85);
    query.exec();
    
    // 一等座
    query.addBindValue(trainId); query.addBindValue("一等座"); query.addBindValue(930.0); query.addBindValue(30); query.addBindValue(12);
    query.exec();

    m_db.commit(); // 提交事务
    qDebug() << "规范化测试数据集已成功注入 SQLite！";
}