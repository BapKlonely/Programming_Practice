#include "datastore.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

DataStore::DataStore(const QString &dbName) {
    qDebug() << "========================================";
    qDebug() << "🚀 SQLite 数据库大管家启动中...";

    // 1. 初始化 SQLite 驱动并设置数据库文件名
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbName);

    // 2. 尝试打开数据库
    if (!m_db.open()) {
        qDebug() << "❌ 致命错误：SQLite 数据库打开失败！" << m_db.lastError().text();
    } else {
        qDebug() << "📂 成功：SQLite 数据库已连接，物理路径:" << dbName;
        // 3. 自动建表并装载数据
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

// 自动建立符合 train.h 规范的数据库表
void DataStore::initTables() {
    QSqlQuery query(m_db);
    
    // 建立 trains 表（字段完全对齐之前编写的 TrainService 查询语句）
    QString createTrainTable = 
        "CREATE TABLE IF NOT EXISTS trains ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "train_no TEXT, "
        "start_station TEXT, "
        "destination_station TEXT, "
        "start_date TEXT, "
        "seat_type TEXT, "
        "rest_seats INTEGER, "
        "status TEXT"
        ");";

    if (!query.exec(createTrainTable)) {
        qDebug() << "❌ 创建 trains 表失败:" << query.lastError().text();
    } else {
        qDebug() << "✅ trains 数据表准备就绪";
    }
}

// 自动注入测试车次（如果表里没数据的话）
void DataStore::insertMockData() {
    QSqlQuery query(m_db);
    
    // 先检查表里是不是已经有数据了，防止每次启动重复插入
    query.exec("SELECT COUNT(*) FROM trains");
    if (query.next() && query.value(0).toInt() > 0) {
        return; // 已有数据，直接返回
    }

    // 插入初始测试车次（复用之前的假数据）
    m_db.transaction(); // 开启事务加速插入
    
    query.prepare("INSERT INTO trains (train_no, start_station, destination_station, start_date, seat_type, rest_seats, status) "
                  "VALUES (:no, :start, :dest, :date, :seat, :rest, :status)");

    // 车次 1：G114 二等座
    query.bindValue(":no", "G114");
    query.bindValue(":start", "北京");
    query.bindValue(":dest", "上海");
    query.bindValue(":date", "2026-07-08");
    query.bindValue(":seat", "二等座");
    query.bindValue(":rest", 85);
    query.bindValue(":status", "running");
    query.exec();

    // 车次 2：G114 一等座
    query.bindValue(":no", "G114");
    query.bindValue(":start", "北京");
    query.bindValue(":dest", "上海");
    query.bindValue(":date", "2026-07-08");
    query.bindValue(":seat", "一等座");
    query.bindValue(":rest", 12);
    query.bindValue(":status", "running");
    query.exec();

    m_db.commit(); // 提交事务
    qDebug() << "🎁 初始测试车次数据已成功注入 SQLite 数据库！";
}