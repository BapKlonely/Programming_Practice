# Database 数据库模块使用说明

> **编写人**：成员 E（集成 + 票务）  
> **适用读者**：成员 A/B/C/D（仅有 C++ 基础，未系统学习 Qt / SQL）  
> **代码位置**：[`mysrc/core/database.h`](../mysrc/core/database.h)、[`mysrc/core/database.cpp`](../mysrc/core/database.cpp)  
> **相关模型**：[`mysrc/models/`](../mysrc/models/)（`user.h`、`train.h`、`order.h`）

---
## 零、当前进度
database模块主要是用来替换之前跑的AI参考模板里的JSON数据库，提供了一个更规范的数据库模式。调用这个库可以先阅读一下database.h头文件里的格式和注释中写的功能，并简单学习SQL的语法了解如何查询修改数据。 
除了这一段其他都是AI跑的，有啥问题可以问我并修改。

## 一、这个模块是干什么的？

火车票系统需要把**用户、车次、余票、订单**等数据保存下来，程序关闭后下次打开还能读到。成员 E 负责的数据库模块做三件事：

1. **打开 SQLite 数据库文件**（一个 `.db` 文件，无需安装 MySQL）
2. **自动创建 7 张业务表**（若表不存在）
3. **首次运行时插入演示数据**（管理员账号、示例车次等）

其他成员**不需要自己写建表 SQL**，只要在程序启动时调用一次 `Database::initialize()`，然后在各自的 Service 里用 SQL 读写数据即可。

### 和 `src/` 里 JSON 版本的区别

| 目录 | 存储方式 | 用途 |
|------|----------|------|
| `src/src/datastore.*` | JSON 文件 | 小组参考模板，成员 B 可参考 UI 原型 |
| **`mysrc/core/database.*`** | **SQLite** | **正式数据层，全队联调使用** |

联调时请使用 **`mysrc`** 中的 Database，不要混用 JSON 版 DataStore。

---

## 二、30 秒快速上手

在 `main.cpp` 或测试程序里，**程序启动后第一件事**：

```cpp
#include "core/database.h"
#include <QCoreApplication>  // 无界面用 QCoreApplication；有界面用 QApplication
#include <QDir>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);  // Qt 模块要求程序里必须有这个对象

    // 1. 数据库文件路径（建议放在 exe 同级的 data/ 目录）
    QString dbPath = QDir(QCoreApplication::applicationDirPath())
                         .filePath("data/ticket.db");

    // 2. 初始化：打开文件 → 建表 → 插入种子数据
    Database &db = Database::instance();
    if (!db.initialize(dbPath)) {
        qDebug() << "数据库初始化失败：" << db.lastError();
        return 1;
    }

    // 3. 之后各 Service 通过 db.connection() 执行 SQL
    // AuthService auth(db);
    // TrainService trains(db);
    // ...

    return app.exec();  // 有界面时；纯测试可直接 return 0
}
```

**验证是否成功**：编译运行 [`mysrc/tests/database_manual.cpp`](../mysrc/tests/database_manual.cpp) 对应的目标 `ticket_test`，控制台应打印用户表、车次表、T999 余票为 1。

---

## 三、零基础必读：SQLite 和 Qt SQL

### 3.1 SQLite 是什么？

- 一种**文件数据库**：整个库就是一个文件，例如 `data/ticket.db`
- 用 **SQL 语言**操作数据（和 MySQL 语法大部分相同）
- 适合课程项目：免部署、零配置

常见 SQL 动词（先记住这四个）：

| SQL | 作用 | 示例 |
|-----|------|------|
| `SELECT` | 查询 | `SELECT * FROM users WHERE username = 'admin'` |
| `INSERT` | 插入 | `INSERT INTO stations (name) VALUES ('北京')` |
| `UPDATE` | 修改 | `UPDATE users SET enabled = 0 WHERE id = 2` |
| `DELETE` | 删除 | `DELETE FROM trains WHERE id = 5` |

### 3.2 Qt 里怎么执行 SQL？

Qt 提供了两个核心类（**不需要** `#include` 额外安装的东西，链接 `Qt6::Sql` 即可）：

```cpp
#include <QSqlDatabase>  // 数据库连接（Database 类已经帮你打开好了）
#include <QSqlQuery>     // 执行 SQL 语句
```

**固定用法模板**（各 Service 里照抄改 SQL 即可）：

```cpp
// 从 Database 单例拿到连接
QSqlDatabase conn = Database::instance().connection();
QSqlQuery query(conn);

// 方式 A：简单查询（无用户输入时用）
if (!query.exec("SELECT id, username FROM users")) {
    qDebug() << "查询失败：" << query.lastError().text();
    return;
}
while (query.next()) {  // 逐行读取结果
    int id = query.value(0).toInt();           // 第 0 列
    QString name = query.value(1).toString();    // 第 1 列
}

// 方式 B：带参数的查询（推荐！防止 SQL 注入）
query.prepare("SELECT * FROM users WHERE username = ?");
query.addBindValue("admin");   // 第一个 ? 绑定为 "admin"
if (!query.exec()) { /* 错误处理 */ }
```

**重要规则**：

- 用户输入（用户名、站名等）**必须用 `?` 占位符 + `addBindValue`**，不要拼字符串
- `query.next()` 第一次调用时会移到第一行；返回 `false` 表示没有更多行
- `query.value(列号)` 或 `query.value("列名")` 取当前行的字段

### 3.3 单例模式 `Database::instance()`

整个程序只需要**一个**数据库连接。`Database` 使用单例：

```cpp
Database &db = Database::instance();  // 全局唯一
db.initialize("data/ticket.db");
```

各 Service 构造函数接收 `Database &` 引用即可，例如：

```cpp
class AuthService {
public:
    explicit AuthService(Database &db) : m_db(db) {}
private:
    Database &m_db;
};
```

---

## 四、Database 类公开接口说明

头文件：[`database.h`](../mysrc/core/database.h)

| 方法 | 返回值 | 功能 | 谁调用 |
|------|--------|------|--------|
| `static Database& instance()` | 引用 | 获取全局唯一 Database 对象 | main、所有 Service |
| `bool initialize(dbFilePath)` | bool | 打开 `.db`、建表、种子数据 | main / AppContext |
| `QSqlDatabase connection() const` | 连接 | 供 `QSqlQuery` 使用 | C/D/E 的 Service 内部 |
| `bool isOpen() const` | bool | 是否已成功初始化 | 启动自检 |
| `QString lastError() const` | 字符串 | 最近一次失败的中文原因 | 调试、弹窗提示 |

### `initialize()` 内部做了什么？

```
initialize("data/ticket.db")
    │
    ├─ 创建 data/ 目录（若不存在）
    ├─ 注册 SQLite 驱动，打开 .db 文件
    ├─ createTables()  →  CREATE TABLE IF NOT EXISTS ...（7 张表）
    └─ seedIfEmpty()   →  若 users 表为空，插入演示账号和车次
```

- **重复调用**：若已打开会先关闭再重新打开（方便测试）
- **种子数据**：只在 `users` 表为空时插入，**不会**每次启动清空你的数据

### 错误处理示例

```cpp
if (!Database::instance().initialize(dbPath)) {
    QString err = Database::instance().lastError();
    // err 可能是："无法打开数据库：..." 或 "建表失败：..."
    QMessageBox::critical(nullptr, "启动失败", err);  // UI 层
    return 1;
}
```

---

## 五、数据表结构（全队共用）

建表 SQL 在 [`database.cpp` 的 `createTables()`](../mysrc/core/database.cpp) 中。下表说明**每张表存什么、谁负责读写**。

### 5.1 `users` — 用户账号（成员 C）

| 列名 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER | 主键，自增 |
| `username` | TEXT | 登录名，**唯一** |
| `password_hash` | TEXT | 密码哈希（C 后续用 SHA256+salt；种子数据暂为明文方便联调） |
| `salt` | TEXT | 密码盐值 |
| `role` | TEXT | `'admin'` 或 `'seller'` |
| `enabled` | INTEGER | `1` 启用，`0` 禁用 |
| `created_at` | TEXT | 创建时间 |

对应 C++ 结构体：[`models/user.h`](../mysrc/models/user.h) 中的 `struct User`

### 5.2 `stations` — 站点（成员 D）

| 列名 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER | 主键 |
| `name` | TEXT | 站名，唯一，如 "北京" |

### 5.3 `trains` — 车次（成员 D）

| 列名 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER | 主键 |
| `train_no` | TEXT | 车次号，如 "G101" |
| `from_station_id` | INTEGER | 外键 → `stations.id` |
| `to_station_id` | INTEGER | 外键 → `stations.id` |
| `depart_time` | TEXT | 出发时刻 "08:00" |
| `arrive_time` | TEXT | 到达时刻 |
| `travel_date` | TEXT | 乘车日期，ISO 格式 "2026-07-08" |
| `base_price` | REAL | 基准票价 |
| `status` | TEXT | `'running'` 运行 / `'suspended'` 停运 |

对应结构体：[`models/train.h`](../mysrc/models/train.h) 中的 `struct Train`

### 5.4 `seat_inventory` — 席位库存（成员 D 配置，成员 E 订票时扣减）

| 列名 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER | 主键 |
| `train_id` | INTEGER | 外键 → `trains.id` |
| `seat_type` | TEXT | 席别，如 "二等座" |
| `total` | INTEGER | 该席别总票数 |
| `sold` | INTEGER | 已售张数 |

**余票计算公式**：`remaining = total - sold`（不要单独存 remaining 列）

同一车次不同席别各占一行，`(train_id, seat_type)` 唯一。

### 5.5 `orders` — 订单（成员 E）

| 列名 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER | 主键 |
| `order_no` | TEXT | 业务订单号，唯一 |
| `train_id` | INTEGER | 外键 → `trains.id` |
| `seller_id` | INTEGER | 外键 → `users.id`（售票员） |
| `passenger_name` | TEXT | 乘车人姓名 |
| `id_card` | TEXT | 证件号 |
| `seat_type` | TEXT | 席别 |
| `ticket_count` | INTEGER | 张数 |
| `price` | REAL | 实付总价 |
| `status` | TEXT | `'paid'` / `'refunded'` |
| `created_at` | TEXT | 下单时间 |

对应结构体：[`models/order.h`](../mysrc/models/order.h) 中的 `struct Order`

### 5.6 `ticket_logs` — 售票操作日志（成员 E，P1）

| 列名 | 类型 | 说明 |
|------|------|------|
| `id` | INTEGER | 主键 |
| `seller_id` | INTEGER | 哪位售票员 |
| `action` | TEXT | `'book'` 或 `'refund'` |
| `detail` | TEXT | 操作详情文字 |
| `created_at` | TEXT | 操作时间 |

### 5.7 `route_edges` — 线路图边（成员 E，P2 Dijkstra）

P2 换乘算法用，表已建好，暂可不写数据。

### 表关系简图

```
stations ──< trains ──< seat_inventory
                │
users ──────────┼──< orders
                │
                └──< route_edges (P2)
users ──< ticket_logs
```

---

## 六、种子数据（首次运行自动插入）

初始化成功后，数据库里会有以下演示数据，**联调可直接使用**：

### 默认账号

| 用户名 | 密码 | role | 说明 |
|--------|------|------|------|
| `admin` | `admin123` | admin | 管理员 |
| `seller` | `seller123` | seller | 售票员 |

### 站点 ID 对照

| id | 站名 |
|----|------|
| 1 | 北京 |
| 2 | 上海 |
| 3 | 广州 |
| 4 | 成都 |
| 5 | 武汉 |
| 6 | 西安 |

### 示例车次

| 车次 | 路线 | 席别 | 总票 | 用途 |
|------|------|------|------|------|
| G101 | 北京→上海 | 二等座 | 120 | 正常购票 demo |
| G305 | 上海→广州 | 一等座 | 80 | 查票 demo |
| D2201 | 成都→武汉 | 二等座 | 100 | |
| K88 | 广州→北京 | 硬卧 | 160 | |
| Z19 | 北京→西安 | 软卧 | 60 | |
| **T999** | 北京→上海 | 二等座 | **1** | **超售测试专用** |

> T999 只有 1 张票：成员 E 的 TicketService 会用它测试「两人同时抢最后一张票」的场景。

---

## 七、各成员如何使用（含完整示例）

### 7.1 成员 C — 登录验证（AuthService）

**目标**：用户输入用户名密码，查 `users` 表验证。

```cpp
#include "core/database.h"
#include "models/user.h"
#include <QSqlQuery>

std::optional<User> AuthService::login(const QString &username, const QString &password)
{
    QSqlQuery query(m_db.connection());
    query.prepare(
        "SELECT id, username, password_hash, salt, role, enabled, created_at "
        "FROM users WHERE username = ?");
    query.addBindValue(username);

    if (!query.exec() || !query.next()) {
        return std::nullopt;  // 用户不存在
    }

    User user;
    user.id = query.value(0).toInt();
    user.username = query.value(1).toString();
    user.passwordHash = query.value(2).toString();
    user.salt = query.value(3).toString();
    user.role = query.value(4).toString();
    user.enabled = query.value(5).toInt() == 1;
    user.createdAt = query.value(6).toString();

    if (!user.enabled) {
        return std::nullopt;  // 账号已禁用
    }

    // TODO: 正式版用 SHA256(password + salt) 与 passwordHash 比较
    // 联调阶段种子数据 password_hash 存的是明文
    if (user.passwordHash != password) {
        return std::nullopt;
    }

    return user;
}
```

**新建售票员**：

```cpp
bool UserService::createSeller(const QString &username, const QString &password)
{
    QSqlQuery query(m_db.connection());
    query.prepare(
        "INSERT INTO users (username, password_hash, salt, role, enabled, created_at) "
        "VALUES (?, ?, '', 'seller', 1, ?)");
    query.addBindValue(username);
    query.addBindValue(password);  // 后续改为哈希
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    return query.exec();
}
```

---

### 7.2 成员 D — 余票查询（TrainService）

**目标**：按出发站、到达站、日期查可售车次及余票。

```cpp
#include "core/database.h"
#include "models/train.h"
#include <QSqlQuery>

QVector<TrainTicketInfo> TrainService::queryRemainingTickets(
    const QString &from, const QString &to, const QDate &date)
{
    QVector<TrainTicketInfo> result;
    QSqlQuery query(m_db.connection());

    // JOIN 三表：车次 + 起终点站名 + 库存
    query.prepare(R"(
        SELECT t.id, t.train_no,
               s1.name AS from_name, s2.name AS to_name,
               t.travel_date, t.depart_time, t.arrive_time,
               si.seat_type, t.base_price, si.total, si.sold
        FROM trains t
        JOIN stations s1 ON t.from_station_id = s1.id
        JOIN stations s2 ON t.to_station_id = s2.id
        JOIN seat_inventory si ON si.train_id = t.id
        WHERE s1.name = ? AND s2.name = ? AND t.travel_date = ?
          AND t.status = 'running'
    )");
    query.addBindValue(from);
    query.addBindValue(to);
    query.addBindValue(date.toString(Qt::ISODate));

    if (!query.exec()) {
        return result;
    }

    while (query.next()) {
        TrainTicketInfo info;
        info.trainId = query.value(0).toInt();
        info.trainNo = query.value(1).toString();
        info.fromStation = query.value(2).toString();
        info.toStation = query.value(3).toString();
        info.travelDate = QDate::fromString(query.value(4).toString(), Qt::ISODate);
        info.departTime = query.value(5).toString();
        info.arriveTime = query.value(6).toString();
        info.seatType = query.value(7).toString();
        info.price = query.value(8).toDouble();
        info.totalSeats = query.value(9).toInt();
        int sold = query.value(10).toInt();
        info.remainingSeats = info.totalSeats - sold;
        result.append(info);
    }
    return result;
}
```

**查询某车次某席别余票**（供成员 E 订票前校验）：

```cpp
int TrainService::getRemainingSeats(int trainId, const QString &seatType)
{
    QSqlQuery query(m_db.connection());
    query.prepare(
        "SELECT total - sold FROM seat_inventory WHERE train_id = ? AND seat_type = ?");
    query.addBindValue(trainId);
    query.addBindValue(seatType);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}
```

**新增车次 + 配置库存**：

```cpp
bool TrainService::addTrain(const Train &train, const QString &seatType, int totalSeats)
{
    QSqlDatabase conn = m_db.connection();
    if (!conn.transaction()) return false;  // 开启事务：要么全成功要么全失败

    QSqlQuery query(conn);
    query.prepare(
        "INSERT INTO trains (train_no, from_station_id, to_station_id, "
        "depart_time, arrive_time, travel_date, base_price, status) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, 'running')");
    query.addBindValue(train.trainNo);
    query.addBindValue(train.fromStationId);
    query.addBindValue(train.toStationId);
    query.addBindValue(train.departTime);
    query.addBindValue(train.arriveTime);
    query.addBindValue(train.travelDate.toString(Qt::ISODate));
    query.addBindValue(train.basePrice);

    if (!query.exec()) {
        conn.rollback();
        return false;
    }

    int trainId = query.lastInsertId().toInt();

    query.prepare("INSERT INTO seat_inventory (train_id, seat_type, total, sold) VALUES (?, ?, ?, 0)");
    query.addBindValue(trainId);
    query.addBindValue(seatType);
    query.addBindValue(totalSeats);

    if (!query.exec()) {
        conn.rollback();
        return false;
    }

    return conn.commit();
}
```

---

### 7.3 成员 E — 订票扣库存（TicketService，事务 + 超售风控）

**核心思路**：在一个事务里同时「检查余票 → 增加 sold → 插入订单」，防止两人同时买到最后一张票。

```cpp
BookResult TicketService::bookTicket(int trainId, const QString &seatType,
                                     const QString &passengerName, const QString &idCard,
                                     int count, int sellerId)
{
    BookResult result;
    QSqlDatabase conn = m_db.connection();

    if (!conn.transaction()) {
        result.message = "无法开启事务";
        return result;
    }

    QSqlQuery query(conn);

    // 1. 原子扣减：只有 remaining >= count 时才更新，并检查影响行数
    query.prepare(
        "UPDATE seat_inventory SET sold = sold + ? "
        "WHERE train_id = ? AND seat_type = ? AND (total - sold) >= ?");
    query.addBindValue(count);
    query.addBindValue(trainId);
    query.addBindValue(seatType);
    query.addBindValue(count);

    if (!query.exec() || query.numRowsAffected() == 0) {
        conn.rollback();
        result.message = "余票不足";
        return result;
    }

    // 2. 生成订单号并插入 orders
    QString orderNo = "ORD" + QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    double price = calculatePrice(trainId, seatType) * count;

    query.prepare(
        "INSERT INTO orders (order_no, train_id, seller_id, passenger_name, id_card, "
        "seat_type, ticket_count, price, status, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 'paid', ?)");
    query.addBindValue(orderNo);
    query.addBindValue(trainId);
    query.addBindValue(sellerId);
    query.addBindValue(passengerName);
    query.addBindValue(idCard);
    query.addBindValue(seatType);
    query.addBindValue(count);
    query.addBindValue(price);
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    if (!query.exec()) {
        conn.rollback();
        result.message = "创建订单失败";
        return result;
    }

    if (!conn.commit()) {
        result.message = "提交事务失败";
        return result;
    }

    result.ok = true;
    result.orderNo = orderNo;
    return result;
}
```

**退票**（恢复库存 + 改订单状态，同样建议包在事务里）：

```cpp
bool TicketService::refundTicket(const QString &orderNo, int sellerId, QString *err)
{
    QSqlDatabase conn = m_db.connection();
    if (!conn.transaction()) {
        if (err) *err = "无法开启事务";
        return false;
    }

    QSqlQuery query(conn);

    // 查订单
    query.prepare("SELECT train_id, seat_type, ticket_count, status FROM orders WHERE order_no = ?");
    query.addBindValue(orderNo);
    if (!query.exec() || !query.next()) {
        conn.rollback();
        if (err) *err = "订单不存在";
        return false;
    }
    if (query.value(3).toString() == "refunded") {
        conn.rollback();
        if (err) *err = "订单已退票";
        return false;
    }

    int trainId = query.value(0).toInt();
    QString seatType = query.value(1).toString();
    int count = query.value(2).toInt();

    // 恢复 sold
    query.prepare("UPDATE seat_inventory SET sold = sold - ? WHERE train_id = ? AND seat_type = ?");
    query.addBindValue(count);
    query.addBindValue(trainId);
    query.addBindValue(seatType);
    query.exec();

    // 改订单状态
    query.prepare("UPDATE orders SET status = 'refunded' WHERE order_no = ?");
    query.addBindValue(orderNo);
    query.exec();

    return conn.commit();
}
```

---

### 7.4 成员 B — UI 层怎么接

**规则：UI 不直接 `#include "core/database.h"`，只调用 Service。**

```cpp
// ✅ 正确：UI 只依赖 Service
class SellerMainWindow : public QMainWindow {
public:
    SellerMainWindow(TicketService &ticket, OrderService &order, TrainService &train, ...);
};

void SellerMainWindow::onBookClicked()
{
    BookResult r = m_ticket.bookTicket(trainId, seatType, name, idCard, count, m_sellerId);
    if (r.ok) {
        QMessageBox::information(this, "成功", "订单号：" + r.orderNo);
    } else {
        QMessageBox::warning(this, "失败", r.message);
    }
}
```

```cpp
// ❌ 错误：UI 里写 SQL
QSqlQuery q(Database::instance().connection());
q.exec("INSERT INTO orders ...");  // 不要这样做
```

**main.cpp 装配示例**（联调阶段）：

```cpp
#include "core/database.h"
#include "services/TicketService.h"
#include "services/TrainService.h"
// #include "services/AuthService.h"  // 成员 C 提供

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QString dbPath = QDir(QApplication::applicationDirPath()).filePath("data/ticket.db");
    Database &db = Database::instance();
    if (!db.initialize(dbPath)) { /* 错误处理 */ return 1; }

    TrainService trainService(db);
    TicketService ticketService(db, trainService);
    // AuthService authService(db);

    // SellerMainWindow w(ticketService, ...);
    // w.show();

    return app.exec();
}
```

---

## 八、常用 SQL 速查

### 按订单号查订单（成员 E / B）

```sql
SELECT o.*, t.train_no, s1.name AS from_station, s2.name AS to_station
FROM orders o
JOIN trains t ON o.train_id = t.id
JOIN stations s1 ON t.from_station_id = s1.id
JOIN stations s2 ON t.to_station_id = s2.id
WHERE o.order_no = ?
```

### 某车次售票统计（P1 StatsService）

```sql
SELECT t.train_no, si.seat_type, si.total, si.sold,
       (si.total - si.sold) AS remaining,
       (SELECT COALESCE(SUM(price), 0) FROM orders
        WHERE train_id = t.id AND status = 'paid') AS revenue
FROM trains t
JOIN seat_inventory si ON si.train_id = t.id
WHERE t.id = ?
```

### 软删除 / 停运车次（成员 D）

```sql
UPDATE trains SET status = 'suspended' WHERE id = ?
-- 停运后 queryRemainingTickets 应加条件 status = 'running'
```

---

## 九、编译与测试

### 9.1 CMake 依赖

你的 `CMakeLists.txt` 需要链接 SQL 模块：

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Sql Widgets)  # Widgets 有界面时需要
target_link_libraries(你的目标 PRIVATE Qt6::Core Qt6::Sql Qt6::Widgets)
```

### 9.2 运行 database 测试

1. 用 Qt Creator 打开 [`mysrc/CMakeLists.txt`](../mysrc/CMakeLists.txt)
2. 构建目标 **`ticket_test`**
3. 运行后检查控制台输出
4. 数据库文件生成在：`build/.../data/test_ticket.db`

也可用 [DB Browser for SQLite](https://sqlitebrowser.org/) 打开 `.db` 文件，直接查看表内容。

### 9.3 自检清单

- [ ] `initialize()` 返回 `true`
- [ ] `users` 表有 admin、seller 两条记录
- [ ] `stations` 表有 6 个站点
- [ ] `trains` 表有 6 条车次
- [ ] T999 的 `total - sold = 1`

---

## 十、协作约定与注意事项

| 规则 | 说明 |
|------|------|
| **UI 不直连 Database** | 成员 B 只调 `*Service` 公开方法 |
| **不修改 `database.cpp` 建表** | 需改表结构先和成员 E 商量，避免冲突 |
| **库存只由 TicketService 扣减** | 成员 D 只配置 `total`，不直接改 `sold`（退票除外由 E 处理） |
| **密码字段** | 种子数据为明文；成员 C 实现哈希后新用户用哈希，老数据可迁移 |
| **外键** | 已 `PRAGMA foreign_keys = ON`，插入时 station_id / train_id 必须存在 |
| **日期格式** | 统一 `yyyy-MM-dd`（`QDate::toString(Qt::ISODate)`） |

### 遇到问题找谁？

| 问题类型 | 联系人 |
|----------|--------|
| 建表、种子数据、事务、订票 SQL | 成员 E |
| users 表、登录逻辑 | 成员 C |
| trains / seat_inventory、查票 | 成员 D |
| 界面怎么调 Service | 成员 B |
| 接口文档、测试用例 | 成员 A |

---

## 十一、文件清单

```
mysrc/
├── core/
│   ├── database.h      ← 公开接口（本文档重点）
│   └── database.cpp    ← 建表 SQL + 种子数据
├── models/
│   ├── user.h          ← 成员 C 主维护
│   ├── train.h         ← 成员 D 主维护
│   └── order.h         ← 成员 E 主维护
├── services/           ← 各 Service 头文件（.cpp 由各负责人实现）
└── tests/
    └── database_manual.cpp   ← 数据库自检程序
```

---

## 十二、下一步计划（成员 E）

- [ ] 实现 `TicketService.cpp`（订票 / 退票 / 超售风控）
- [ ] 实现 `OrderService.cpp`（订单查询）
- [ ] P1：`TicketLogService`、`StatsService`
- [ ] P2：`RouteGraph`（Dijkstra）、动态票价
- [ ] 与成员 B/C/D 联调，合并进主工程 `main.cpp`

如有接口疑问，请在群里 @成员 E，或查看 [`票务集成模块设计方案.md`](./票务集成模块设计方案.md)。
