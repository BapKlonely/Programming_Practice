# 火车票管理系统 - 后端核心服务与数据底座分支

---

## 一、重要与独特文件解析

本分支对以下核心源码进行了决定性的重构与编写，这也是本分支最具技术含金量的部分：

### 1. `TrainService.cpp`
* **模糊查询优化**：重写了原本为空壳的 `searchTrains` 接口，引入 SQL 的 `LIKE` 机制，支持车次号、出发站的灵活模糊匹配。
* **精确余票检索**：实现了 `queryRemainingTickets` 接口，严格根据日期、始发/终到站，从 SQLite 数据库中实时过滤出指定席别与余票。
  * **大小写自动兼容**：内部执行 `trainNo.toUpper()`。用户在前端输入小写 `g114` 或 `d514`，后端会自动清洗并完美匹配数据库中的大写标准车次 `G114`。
  * **前后空格自动削除**：对城市名、车次号强力执行 `.trimmed()` 过滤，自动砍掉用户手抖多打的空格（如输入 `"  北京  "` 自动清洗为 `"北京"`）。
  * **同站与空输入拦截**：在检索前进行前置校验，若出发地与目的地相同，直接拦截并返回空结果，避免盲目查询数据库。

### 2. `datastore.cpp` / `datastore.h`
* **SQLite 核心重构**：放弃JSON 读写，改用 `QSqlDatabase` 驱动管理。程序启动时会自动检测并创建本地物理数据库 `train_system.db`。
* **自动建表与事务优化**：内建 `initTables()` 自动创建符合项目的 `trains` 数据表；内建 `insertMockData()` 并引入 SQL 事务（`transaction`），在数据库首次创建时，安全、极速地注入全套初始车次测试数据。
* **高公开性数据模型**：解除了原有的私有变量限制，全面支持更利于团队快速传递数据的结构体模型。

---

## 三、接口调用与联调指南

为了方便负责前端界面的**成员 B** 顺畅调用，请按照以下规范进行业务对接：

### 1. 核心接口 API 调用示例

#### ① 车次模糊查询 (`searchTrains`)
* **作用**：用于主界面搜索框的模糊检索。
* **调用代码**：

```cpp
// 哪怕用户输入的是 "  g114  "（带空格且小写），后端服务也能完美清洗并返回
QVector<Train> results = trainService.searchTrains("  g114  ", "北京", QDate(2026, 7, 8));

for (const Train& t : results) {
    // 严格对应 train.h 变量名
    qDebug() << "匹配车次:" << t.trainNo << " 运行状态:" << t.status;
}
```

#### ② 购票页面余票精确检索 (`queryRemainingTickets`)
* **作用**：用于旅客选择完出发地、目的地后的精确刷票列表呈现。
* **调用代码**：

```cpp
QVector<TrainTicketInfo> tickets = trainService.queryRemainingTickets("成都", "西安", QDate(2026, 7, 8));

for (const TrainTicketInfo& info : tickets) {
    // 严格对应 train.h 变量名：availableSeats(余票), seatType(座位类型)
    qDebug() << "车次:" << info.trainNo
             << " 席别:" << info.seatType
             << " 余票:" << info.availableSeats;
}
```
