#include "services/TrainService.h"
#include "core/database.h"
#include <QSqlError>
#include <QSqlQuery>

/**
 * @brief 构造函数：实现持久层与业务层的依赖注入
 * @param db 传入外部已经初始化完毕的数据库大管家引用，保证全队共用同一个数据库连接连接
 */
TrainService::TrainService(Database &db)
    : m_db(db) // 初始化列表中绑定内部数据库引用
{
}

/**
 * @brief 功能一：车次模糊综合搜索看板
 * @details 采用高级动态 SQL 技巧，当某个输入项为空时，该过滤条件在数据库端自动失效。
 */
QVector<Train> TrainService::searchTrains(const QString &trainNo, const QString &from, const QDate &date)
{
    //车次强转大写，站名剔除前后空格，规避用户误输入导致的检索失败
    const QString cleanTrainNo = trainNo.toUpper().trimmed();
    const QString cleanFrom = from.trimmed();

    QVector<Train> result;
    //底层数据库若未正常打开，立即熔断返回空容器，坚决防止后续 SQL 执行引发程序崩溃
    if (!m_db.isOpen()) {
        return result;
    }

    //使用全局数据库连接句柄创建查询对象
    QSqlQuery query(m_db.connection());
    
    //使用 QStringLiteral 宏包裹大段 SQL。
    //让字符串在编译期直接放入静态数据段，彻底免除运行时频繁分配/销毁临时 QString 对象的系统开销。
    query.prepare(QStringLiteral(
        "SELECT t.*, s1.name AS from_name, s2.name AS to_name "
        "FROM trains t "
        "JOIN stations s1 ON t.from_station_id = s1.id " //多表联查：将数字化始发站 ID 翻译为人类可读的真实站名
        "JOIN stations s2 ON t.to_station_id = s2.id "   //多表联查：将数字化终点站 ID 翻译为人类可读的真实站名
        "WHERE (t.train_no LIKE :trainNo OR :trainNoEmpty) AND " //若车次为空，:trainNoEmpty 为真，则该行条件恒成立
        "(s1.name = :from OR :fromEmpty) AND "
        "(t.travel_date = :date OR :dateEmpty)"));

    //严格使用参数化绑定，将“SQL 命令结构”与“用户输入数据”在底层彻底隔离
    // 通过 .arg() 动态构造符合 SQLite 语法规则的 %关键字% 模糊查询字符串
    query.bindValue(QStringLiteral(":trainNo"), QStringLiteral("%%1%").arg(cleanTrainNo));
    query.bindValue(QStringLiteral(":trainNoEmpty"), cleanTrainNo.isEmpty());
    query.bindValue(QStringLiteral(":from"), cleanFrom);
    query.bindValue(QStringLiteral(":fromEmpty"), cleanFrom.isEmpty());
    query.bindValue(QStringLiteral(":date"), date.isValid() ? date.toString(QStringLiteral("yyyy-MM-dd")) : QString());
    query.bindValue(QStringLiteral(":dateEmpty"), !date.isValid());

    //执行查询并进行关系对象映射
    if (query.exec()) {
        while (query.next()) {
            Train t;
            // 将关系型数据库的单行记录映射解析到 C++ 的 Train 结构体实例中
            t.id = query.value(QStringLiteral("id")).toInt();
            t.trainNo = query.value(QStringLiteral("train_no")).toString();
            t.fromStationId = query.value(QStringLiteral("from_station_id")).toInt();
            t.toStationId = query.value(QStringLiteral("to_station_id")).toInt();
            t.fromStation = query.value(QStringLiteral("from_name")).toString(); // 提取JOIN翻译出的始发站名
            t.toStation = query.value(QStringLiteral("to_name")).toString();     // 提取JOIN翻译出的终点站名
            t.departTime = query.value(QStringLiteral("depart_time")).toString();
            t.arriveTime = query.value(QStringLiteral("arrive_time")).toString();
            t.travelDate = query.value(QStringLiteral("travel_date")).toDate();
            t.basePrice = query.value(QStringLiteral("base_price")).toDouble();
            t.status = query.value(QStringLiteral("status")).toString();
            result.append(t); // 将实体塞入容器，准备返回给前端展示
        }
    }
    return result;
}

/**
 * @brief 功能二：两点间实时余票精确检索（供购票界面刷表）
 * @details 将车次主表、站点表、以及席别库存表进行强关联，动态计算每种席别的剩余座位。
 */
QVector<TrainTicketInfo> TrainService::queryRemainingTickets(const QString &from, const QString &to, const QDate &date)
{
    const QString cleanFrom = from.trimmed();
    const QString cleanTo = to.trimmed();

    QVector<TrainTicketInfo> result;
    //若始发站与终点站相同，或核心参数缺失，判定为非法请求，直接拦截打回，杜绝无效的数据库开销
    if (cleanFrom.isEmpty() || cleanTo.isEmpty() || cleanFrom == cleanTo) {
        return result;
    }

    QSqlQuery query(m_db.connection());
    query.prepare(QStringLiteral(
        "SELECT t.id AS t_id, t.train_no, s1.name AS from_name, s2.name AS to_name, "
        "t.travel_date, t.depart_time, t.arrive_time, t.base_price, "
        "i.seat_type, i.total AS total_seats, (i.total - i.sold) AS remaining_seats " //余票动态计算：剩余座位 = 总数(total) - 已售(sold)
        "FROM trains t "
        "JOIN seat_inventory i ON t.id = i.train_id " //三表强关联：车次主表到席别库存表（打散一行车次为多条席别记录）
        "JOIN stations s1 ON t.from_station_id = s1.id "
        "JOIN stations s2 ON t.to_station_id = s2.id "
        "WHERE s1.name = :from AND s2.name = :to AND t.travel_date = :date "
        "AND t.status = 'running'")); //只有正常运行（running）的车次才允许进入购票看板，停运车次在此被过滤

    query.bindValue(QStringLiteral(":from"), cleanFrom);
    query.bindValue(QStringLiteral(":to"), cleanTo);
    query.bindValue(QStringLiteral(":date"), date.toString(QStringLiteral("yyyy-MM-dd")));

    if (query.exec()) {
        while (query.next()) {
            TrainTicketInfo info;
            //严格将多表打散的宽表记录映射至专门用于前端表格渲染的 TrainTicketInfo 结构体
            info.trainId = query.value(QStringLiteral("t_id")).toInt();
            info.trainNo = query.value(QStringLiteral("train_no")).toString();
            info.fromStation = query.value(QStringLiteral("from_name")).toString();
            info.toStation = query.value(QStringLiteral("to_name")).toString();
            info.travelDate = query.value(QStringLiteral("travel_date")).toDate();
            info.departTime = query.value(QStringLiteral("depart_time")).toString();
            info.arriveTime = query.value(QStringLiteral("arrive_time")).toString();
            info.seatType = query.value(QStringLiteral("seat_type")).toString();
            info.price = query.value(QStringLiteral("base_price")).toDouble(); // 计价基准对齐
            info.totalSeats = query.value(QStringLiteral("total_seats")).toInt();
            info.remainingSeats = query.value(QStringLiteral("remaining_seats")).toInt(); // 绑定计算后的最新余票
            result.append(info);
        }
    }
    return result;
}

/**
 * @brief 功能三：发布/发布新车次
 */
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
    query.bindValue(QStringLiteral(":status"), t.status.isEmpty() ? QStringLiteral("running") : t.status); // 默认设为正常运行

    const bool ok = query.exec();
    //高级错误处理：错误冒泡机制
    // 一旦SQL执行失败且外部传入了有效的报错指针，就将SQLite原生的详细错误文本塞回去，方便上层UI弹窗精准报错
    if (!ok && err) {
        *err = query.lastError().text(); 
    }
    return ok;
}

/**
 * @brief 功能四：更新车次全量信息
 */
bool TrainService::updateTrain(const Train &t, QString *err)
{
    QSqlQuery query(m_db.connection());
    query.prepare(QStringLiteral(
        "UPDATE trains SET train_no=:no, from_station_id=:fromId, to_station_id=:toId, "
        "depart_time=:dep, arrive_time=:arr, travel_date=:date, base_price=:price, status=:status "
        "WHERE id=:id")); //精确锁定主键id进行记录覆盖
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

/**
 * @brief 功能五：车次物理下架
 */
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

/**
 * @brief 功能六：快捷微调车次状态（用于突发停运、晚点调度）
 */
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

/**
 * @brief 功能七：管理员席别与总座位配置录入
 */
bool TrainService::configureSeats(int trainId, const QString &seatType, int total, QString *err)
{
    QSqlQuery query(m_db.connection());
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

/**
 * @brief 功能八：风控拦截——校验指定车次此时是否允许买票
 */
bool TrainService::isTrainBookable(int trainId)
{
    QSqlQuery query(m_db.connection());
    query.prepare(QStringLiteral("SELECT status FROM trains WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), trainId);
    if (query.exec() && query.next()) {
        //只有明确状态为running才可以售票。如果被管理员切为suspended停运，立刻拒绝
        return query.value(0).toString() == QStringLiteral("running");
    }
    return false; // 车次不存在直接禁止售票
}

/**
 * @brief 功能九：风控拦截——实时抓取指定席别最精确的剩余座位绝对值
 * @note 本函数专供成员 E 的 TicketService 订票前置风控拦截，防止多窗口高并发抢票导致超卖。
 */
int TrainService::getRemainingSeats(int trainId, const QString &seatType)
{
    QSqlQuery query(m_db.connection());
    //实时余票 = total - sold
    query.prepare(QStringLiteral(
        "SELECT total - sold FROM seat_inventory WHERE train_id = :id AND seat_type = :type"));
    query.bindValue(QStringLiteral(":id"), trainId);
    query.bindValue(QStringLiteral(":type"), seatType.trimmed());
    if (query.exec() && query.next()) {
        return query.value(0).toInt(); // 抛出数据库中最真实的剩余车票张数
    }
    return 0; // 一旦发生异常或没查到该席别，返回0张票，采取最安全的安全风控策略
}

/**
 * @brief 功能十：根据唯一 ID 提取单车次实体详情
 * @note 专供底层联调及订单关联调用，采用C++17的std::optional规范。
 */
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
        return t; //查到时，将Train对象包裹进optional中返回
    }
    return std::nullopt; //没查到时，安全返回空标识，彻底终结了以往返回nullptr导致的空指针系统崩溃顽疾
}