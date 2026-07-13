#include "services/TicketLogService.h"

#include "core/database.h"

#include <QSqlError>
#include <QSqlQuery>

namespace {

/**
 * @brief 将查询结果的一行映射为 TicketLog
 *
 * SELECT 列顺序必须与 kLogSelectSql 中字段别名一致。
 */
TicketLog rowToTicketLog(const QSqlQuery &query)
{
    TicketLog log;
    log.id = query.value(QStringLiteral("id")).toInt();
    log.sellerId = query.value(QStringLiteral("seller_id")).toInt();
    log.sellerUsername = query.value(QStringLiteral("username")).toString();
    log.action = query.value(QStringLiteral("action")).toString();
    log.detail = query.value(QStringLiteral("detail")).toString();
    log.createdAt = query.value(QStringLiteral("created_at")).toString();
    return log;
}

/*
    列表查询的SQL指令
    使用 LEFT JOIN 而非 INNER JOIN：即使用户被删，日志行仍可显示（用户名为空）。
 */
const char *kLogSelectSql = R"SQL(
    SELECT tl.id, tl.seller_id, tl.action, tl.detail, tl.created_at,
           u.username
    FROM ticket_logs tl
    LEFT JOIN users u ON tl.seller_id = u.id
)SQL";

bool isValidAction(const QString &action)
{
    return action == QStringLiteral("book") || action == QStringLiteral("refund");
}

} // namespace

TicketLogService::TicketLogService(Database &db): m_db(db){}

int TicketLogService::normalizeLimit(int limit)
{
    if (limit <= 0) {
        return 100;
    }
    if (limit > 1000) {
        return 1000;
    }
    return limit;
}

QVector<TicketLog> TicketLogService::queryLogs(const QString &whereClause,
                                               int sellerId,
                                               bool bindSellerId,
                                               int limit)
{
    QVector<TicketLog> result;
    if (!m_db.isOpen()) {
        return result;
    }

    const int safeLimit = normalizeLimit(limit);

    QSqlQuery query(m_db.connection());
    QString sql = QString::fromUtf8(kLogSelectSql);
    if (!whereClause.isEmpty()) {
        sql += QStringLiteral(" WHERE ") + whereClause;
    }
    sql += QStringLiteral(" ORDER BY tl.created_at DESC, tl.id DESC LIMIT :lim");

    query.prepare(sql);
    if (bindSellerId) {
        query.bindValue(QStringLiteral(":sellerId"), sellerId);
    }
    query.bindValue(QStringLiteral(":lim"), safeLimit);

    if (!query.exec()) {
        return result;
    }

    while (query.next()) {
        result.append(rowToTicketLog(query));
    }
    return result;
}

QVector<TicketLog> TicketLogService::listBySeller(int sellerId, int limit)
{
    if (sellerId <= 0) {
        return {};
    }
    return queryLogs(QStringLiteral("tl.seller_id = :sellerId"), sellerId, true, limit);
}

QVector<TicketLog> TicketLogService::listAll(int limit)
{
    return queryLogs(QString(), 0, false, limit);
}

QVector<TicketLog> TicketLogService::listByAction(const QString &action, int limit)
{
    const QString trimmed = action.trimmed();
    if (!isValidAction(trimmed)) {
        return {};
    }

    QVector<TicketLog> result;
    if (!m_db.isOpen()) {
        return result;
    }

    const int safeLimit = normalizeLimit(limit);

    QSqlQuery query(m_db.connection());
    query.prepare(QString::fromUtf8(kLogSelectSql)
                  + QStringLiteral(" WHERE tl.action = :action "
                                   "ORDER BY tl.created_at DESC, tl.id DESC LIMIT :lim"));
    query.bindValue(QStringLiteral(":action"), trimmed);
    query.bindValue(QStringLiteral(":lim"), safeLimit);

    if (!query.exec()) {
        return result;
    }

    while (query.next()) {
        result.append(rowToTicketLog(query));
    }
    return result;
}

int TicketLogService::countAll()
{
    if (!m_db.isOpen()) {
        return -1;
    }

    QSqlQuery query(m_db.connection());
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM ticket_logs"))) {
        return -1;
    }
    if (!query.next()) {
        return -1;
    }
    return query.value(0).toInt();
}

int TicketLogService::countBySeller(int sellerId)
{
    if (sellerId <= 0 || !m_db.isOpen()) {
        return -1;
    }

    QSqlQuery query(m_db.connection());
    query.prepare(QStringLiteral("SELECT COUNT(*) FROM ticket_logs WHERE seller_id = ?"));
    query.addBindValue(sellerId);
    if (!query.exec() || !query.next()) {
        return -1;
    }
    return query.value(0).toInt();
}
