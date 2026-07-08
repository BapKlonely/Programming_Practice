/**
 * @file database.cpp
 * @brief Database 类的实现：建表 SQL + 种子数据
 *
 * 【阅读顺序建议】
 * 1. initialize()     — 入口：打开文件 → 建表 → 种子数据
 * 2. createTables()   — 所有 CREATE TABLE 语句
 * 3. seedIfEmpty()    — 第一次运行时的 INSERT 演示数据
 */

#include "database.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

// ---------------------------------------------------------------------------
// 单例
// ---------------------------------------------------------------------------

Database &Database::instance()
{
    // C++11 起，函数内 static 变量只会初始化一次，线程安全
    static Database db;
    return db;
}

// ---------------------------------------------------------------------------
// 公开接口
// ---------------------------------------------------------------------------

bool Database::initialize(const QString &dbFilePath)
{
    m_lastError.clear();

    // 若之前已经打开过，先关闭（方便测试程序多次调用）
    if (m_open) {
        QSqlDatabase::database(m_connectionName).close();
        QSqlDatabase::removeDatabase(m_connectionName);
        m_open = false;
    }

    // 确保 data 目录存在（例如 data/ticket.db 需要先创建 data 文件夹）
    QFileInfo fileInfo(dbFilePath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(QStringLiteral("."))) {
            return fail(QStringLiteral("无法创建数据库目录：%1").arg(dir.absolutePath()));
        }
    }

    // Qt 要求：每个数据库连接必须有一个唯一的“连接名”
    // 这里用 UUID 避免重复打开时冲突
    m_connectionName = QStringLiteral("ticket_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    m_dbFilePath = fileInfo.absoluteFilePath();

    {
        // 注册 SQLite 驱动并设置数据库文件路径
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
        db.setDatabaseName(m_dbFilePath);

        if (!db.open()) {
            return fail(QStringLiteral("无法打开数据库：%1").arg(db.lastError().text()));
        }
    }

    if (!createTables()) {
        return false;
    }
    if (!seedIfEmpty()) {
        return false;
    }

    m_open = true;
    return true;
}

QSqlDatabase Database::connection() const
{
    // 通过连接名取回之前 addDatabase 注册的那个连接
    return QSqlDatabase::database(m_connectionName);
}

bool Database::isOpen() const
{
    return m_open && connection().isOpen();
}

QString Database::lastError() const
{
    return m_lastError;
}

// ---------------------------------------------------------------------------
// 内部：错误处理
// ---------------------------------------------------------------------------

bool Database::fail(const QString &message)
{
    m_lastError = message;
    return false;
}

// ---------------------------------------------------------------------------
// 内部：建表
// ---------------------------------------------------------------------------

bool Database::createTables()
{
    QSqlQuery query(connection());

    /**
     * 【关于 exec()】
     * query.exec("SQL字符串") 执行一条 SQL。
     * 返回 false 表示语法错误或约束冲突，用 lastError() 查看详情。
     *
     * 【关于外键】
     * PRAGMA foreign_keys = ON 开启 SQLite 外键检查（默认可能是关的）。
     */
    if (!query.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) {
        return fail(QStringLiteral("设置外键失败：%1").arg(query.lastError().text()));
    }

    // ----- users：成员 C 的账号模块使用 -----
    const char *sqlUsers = R"SQL(
        CREATE TABLE IF NOT EXISTS users (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            username      TEXT    NOT NULL UNIQUE,
            password_hash TEXT    NOT NULL,
            salt          TEXT    NOT NULL DEFAULT '',
            role          TEXT    NOT NULL CHECK(role IN ('admin', 'seller')),
            enabled       INTEGER NOT NULL DEFAULT 1,
            created_at    TEXT    NOT NULL
        )
    )SQL";
    /*SQL解析：
        id:自增主键，代表用户编号;username:非空不可重复用户名
        password_hash:非空密码的哈希值;role:在售票员、管理员中选择
        salt:盐值是一串随机生成的字符串,将salt文本与密码融合后再哈希，可以防止黑客暴力尝试常规密码破解
        enabled:账号是否启用 created_at:创建时间
    */

    // ----- stations / trains / seat_inventory：成员 D 的车次模块使用 -----
    const char *sqlStations = R"SQL(
        CREATE TABLE IF NOT EXISTS stations (
            id   INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT    NOT NULL UNIQUE
        )
    )SQL";//只存id信息和车站名

    const char *sqlTrains = R"SQL(
        CREATE TABLE IF NOT EXISTS trains (
            id               INTEGER PRIMARY KEY AUTOINCREMENT,
            train_no         TEXT    NOT NULL,
            from_station_id  INTEGER NOT NULL,
            to_station_id    INTEGER NOT NULL,
            depart_time      TEXT    NOT NULL,
            arrive_time      TEXT    NOT NULL,
            travel_date      TEXT    NOT NULL,
            base_price       REAL    NOT NULL DEFAULT 0,
            status           TEXT    NOT NULL DEFAULT 'running'
                CHECK(status IN ('running', 'suspended')),
            FOREIGN KEY(from_station_id) REFERENCES stations(id),
            FOREIGN KEY(to_station_id)   REFERENCES stations(id)
        )
    )SQL";
    /*SQL解析:
        status:车辆运行状态,默认running,可切换至suspended;
        外键检查:起终点必须是stations数据库内存在的车站
    */

    const char *sqlSeatInventory = R"SQL(
        CREATE TABLE IF NOT EXISTS seat_inventory (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            train_id  INTEGER NOT NULL,
            seat_type TEXT    NOT NULL,
            total     INTEGER NOT NULL CHECK(total >= 0),
            sold      INTEGER NOT NULL DEFAULT 0 CHECK(sold >= 0),
            UNIQUE(train_id, seat_type),
            FOREIGN KEY(train_id) REFERENCES trains(id)
        )
    )SQL";

    // ----- orders / ticket_logs：成员 E 的票务模块使用 -----
    const char *sqlOrders = R"SQL(
        CREATE TABLE IF NOT EXISTS orders (
            id             INTEGER PRIMARY KEY AUTOINCREMENT,
            order_no       TEXT    NOT NULL UNIQUE,
            train_id       INTEGER NOT NULL,
            seller_id      INTEGER NOT NULL,
            passenger_name TEXT    NOT NULL,
            id_card        TEXT    NOT NULL,
            seat_type      TEXT    NOT NULL,
            ticket_count   INTEGER NOT NULL DEFAULT 1 CHECK(ticket_count > 0),
            price          REAL    NOT NULL,
            status         TEXT    NOT NULL DEFAULT 'paid'
                CHECK(status IN ('paid', 'refunded')),
            created_at     TEXT    NOT NULL,
            FOREIGN KEY(train_id)  REFERENCES trains(id),
            FOREIGN KEY(seller_id) REFERENCES users(id)
        )
    )SQL";

    const char *sqlTicketLogs = R"SQL(
        CREATE TABLE IF NOT EXISTS ticket_logs (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            seller_id  INTEGER NOT NULL,
            action     TEXT    NOT NULL CHECK(action IN ('book', 'refund')),
            detail     TEXT    NOT NULL,
            created_at TEXT    NOT NULL,
            FOREIGN KEY(seller_id) REFERENCES users(id)
        )
    )SQL";

    // ----- route_edges：P2 换乘算法用，先建好表 -----
    const char *sqlRouteEdges = R"SQL(
        CREATE TABLE IF NOT EXISTS route_edges (
            id               INTEGER PRIMARY KEY AUTOINCREMENT,
            from_station_id  INTEGER NOT NULL,
            to_station_id    INTEGER NOT NULL,
            train_id         INTEGER NOT NULL,
            weight           REAL    NOT NULL DEFAULT 1,
            FOREIGN KEY(from_station_id) REFERENCES stations(id),
            FOREIGN KEY(to_station_id)   REFERENCES stations(id),
            FOREIGN KEY(train_id)        REFERENCES trains(id)
        )
    )SQL";

    const QStringList statements = {//相当于vector<string>,动态处理qstring信息
        QString::fromUtf8(sqlUsers),//将原生qstring转为utf-8编码
        QString::fromUtf8(sqlStations),
        QString::fromUtf8(sqlTrains),
        QString::fromUtf8(sqlSeatInventory),
        QString::fromUtf8(sqlOrders),
        QString::fromUtf8(sqlTicketLogs),
        QString::fromUtf8(sqlRouteEdges),
    };

    for (const QString &sql : statements) {
        if (!query.exec(sql)) {
            return fail(QStringLiteral("建表失败：%1\nSQL: %2").arg(query.lastError().text(), sql.left(80)));
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// 内部：种子数据（仅首次运行插入）
// ---------------------------------------------------------------------------

bool Database::seedIfEmpty()
{
    QSqlQuery query(connection());

    // 判断是否已经 seed 过：看 users 表有没有记录
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM users"))) {
        return fail(QStringLiteral("检查用户表失败：%1").arg(query.lastError().text()));
    }
    query.next();
    if (query.value(0).toInt() > 0) {
        return true;  // 已有数据，跳过
    }

    const QString now = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    /**
     * 【关于占位密码】
     * 成员 C 后续会用 SHA256+salt 存真正哈希。
     * 这里 password_hash 暂时存明文 "admin123" / "seller123" 方便联调前测试。
     * 正式版本请由 AuthService 替换。
     */
    struct UserSeed {
        const char *username;
        const char *password;
        const char *role;
    };
    const UserSeed users[] = {
        {"admin",  "admin123",  "admin"},
        {"seller", "seller123", "seller"},
    };

    for (const UserSeed &u : users) {//循环插入所有已知users的账号信息
        query.prepare(QStringLiteral(
            "INSERT INTO users (username, password_hash, salt, role, enabled, created_at) "
            "VALUES (?, ?, '', ?, 1, ?)"));//预编译SQL,?为占位符,salt测试环境为空,enable默认为1
        query.addBindValue(QString::fromUtf8(u.username));//为占位符填充数据
        query.addBindValue(QString::fromUtf8(u.password));
        query.addBindValue(QString::fromUtf8(u.role));
        query.addBindValue(now);
        if (!query.exec()) {
            return fail(QStringLiteral("插入用户失败：%1").arg(query.lastError().text()));
        }
    }

    // ----- 站点 -----
    const char *stationNames[] = {"北京", "上海", "广州", "成都", "武汉", "西安"};
    for (const char *name : stationNames) {
        query.prepare(QStringLiteral("INSERT INTO stations (name) VALUES (?)"));
        query.addBindValue(QString::fromUtf8(name));
        if (!query.exec()) {
            return fail(QStringLiteral("插入站点失败：%1").arg(query.lastError().text()));
        }
    }

    // 站点 id 与名称对应（按插入顺序从 1 开始）
    // 1北京 2上海 3广州 4成都 5武汉 6西安
    const QDate tomorrow = QDate::currentDate().addDays(1);
    const QDate dayAfter = QDate::currentDate().addDays(2);
    const QDate day3 = QDate::currentDate().addDays(3);//模拟发车日期不同的车次信息

    /**
     * 【车次种子说明】
     * TrainSeed 描述一趟车；库存单独插入 seat_inventory。
     * T999 故意只给 1 张票，供第 3 步测试“超售拦截”。
     */
    struct TrainSeed {
        const char *trainNo;
        int fromId;
        int toId;
        const char *depart;
        const char *arrive;
        QDate date;
        double price;
        const char *seatType;
        int totalSeats;
    };

    const TrainSeed trains[] = {
        {"G101",  1, 2, "08:00", "12:36", tomorrow, 553.0,  "二等座", 120},
        {"G305",  2, 3, "09:20", "16:10", tomorrow, 988.0,  "一等座", 80},
        {"D2201", 4, 5, "07:15", "15:42", dayAfter, 356.0,  "二等座", 100},
        {"K88",   3, 1, "18:45", "10:30", dayAfter, 426.5,  "硬卧",   160},
        {"Z19",   1, 6, "20:40", "08:31", day3,     415.0,  "软卧",   60},
        {"T999",  1, 2, "14:00", "18:00", tomorrow, 100.0,  "二等座", 1},  // 仅 1 张票，测超售
    };//结构体存储车次信息

    for (const TrainSeed &t : trains) {
        query.prepare(QStringLiteral(
            "INSERT INTO trains "
            "(train_no, from_station_id, to_station_id, depart_time, arrive_time, travel_date, base_price, status) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, 'running')"));
        query.addBindValue(QString::fromUtf8(t.trainNo));
        query.addBindValue(t.fromId);
        query.addBindValue(t.toId);
        query.addBindValue(QString::fromUtf8(t.depart));
        query.addBindValue(QString::fromUtf8(t.arrive));
        query.addBindValue(t.date.toString(Qt::ISODate));
        query.addBindValue(t.price);
        if (!query.exec()) {
            return fail(QStringLiteral("插入车次失败：%1").arg(query.lastError().text()));
        }

        // lastInsertId() 返回刚插入行的自增 id
        const int trainId = query.lastInsertId().toInt();

        query.prepare(QStringLiteral(
            "INSERT INTO seat_inventory (train_id, seat_type, total, sold) VALUES (?, ?, ?, 0)"));
        query.addBindValue(trainId);
        query.addBindValue(QString::fromUtf8(t.seatType));
        query.addBindValue(t.totalSeats);
        if (!query.exec()) {
            return fail(QStringLiteral("插入库存失败：%1").arg(query.lastError().text()));
        }
    }

    return true;
}
