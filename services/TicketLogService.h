#pragma once

/**
 * @file TicketLogService.h
 * @brief 售票操作日志查询服务（成员 E，P1）
 *
 * 【职责边界 — 解耦设计】
 *
 *   ┌─────────────┐     事务内 INSERT      ┌──────────────┐
 *   │ TicketService │ ──────────────────► │ ticket_logs  │
 *   │ (写日志)      │                      │   (SQLite)   │
 *   └─────────────┘                      └──────┬───────┘
 *                                               │ SELECT
 *                                               ▼
 *                                        ┌──────────────┐
 *                                        │TicketLogService│
 *                                        │  (只读查询)    │
 *                                        └──────┬───────┘
 *                                               │
 *                                               ▼
 *                                        ┌──────────────┐
 *                                        │ 成员 B UI    │
 *                                        │ 日志列表页   │
 *                                        └──────────────┘
 *
 * - TicketLogService **只读** ticket_logs，不修改订单、不碰库存。
 * - **不依赖** TicketService / OrderService / TrainService 头文件。
 * - **不依赖** AuthService；sellerId 由 UI 传入（来自登录后的 User.id）。
 * - 写日志仍在 TicketService 事务内完成，保证「订单与日志同时成功或同时回滚」。
 *
 * 【成员 B 怎么用？】
 * @code
 *   TicketLogService logs(db);
 *   QVector<TicketLog> recent = logs.listBySeller(currentUser.id, 50);
 *   QVector<TicketLog> all = logs.listAll(100);
 * @endcode
 */

#include "models/ticket_log.h"

#include <QVector>

class Database;

class TicketLogService
{
public:
    explicit TicketLogService(Database &db);

    /**
     * @brief 查询某操作员最近的操作记录（按时间倒序，新的在前）
     *
     * @param sellerId  users.id，即登录管理员的 id
     * @param limit     最多返回条数；<=0 时内部按 100 处理
     * @return 空列表表示无记录或数据库未打开
     */
    QVector<TicketLog> listBySeller(int sellerId, int limit = 100);

    /**
     * @brief 管理员查看全站最近操作（P1 简版，不做复杂权限校验）
     *
     * @param limit  最多返回条数；<=0 时内部按 100 处理
     */
    QVector<TicketLog> listAll(int limit = 100);

    /**
     * @brief 按操作类型筛选（"book" 或 "refund"）
     *
     * @param action  必须为 book / refund，否则返回空列表
     * @param limit   最多返回条数
     */
    QVector<TicketLog> listByAction(const QString &action, int limit = 100);

    /**
     * @brief 统计 ticket_logs 表总行数（自检 / 管理端概览）
     * @return 失败或未打开时返回 -1
     */
    int countAll();

    /**
     * @brief 统计某操作员的日志条数
     * @return 失败或未打开时返回 -1
     */
    int countBySeller(int sellerId);

private:
    Database &m_db;

    /** @brief 规范化 limit，避免 SQL 传入非法值 */
    static int normalizeLimit(int limit);

    /**
     * @brief 执行带 JOIN users 的列表查询（内部共用）
     * @param whereClause  不含 WHERE 关键字，如 "tl.seller_id = :sellerId"
     * @param bindSellerId 若 where 需要 :sellerId，传 true 并配合 sellerId 参数
     */
    QVector<TicketLog> queryLogs(const QString &whereClause,
                                 int sellerId,
                                 bool bindSellerId,
                                 int limit);
};
