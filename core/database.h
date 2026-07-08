#pragma once

/**
 * @file database.h
 * @brief 数据库连接与建表（成员 E 负责，全队共用）
 *
 * 【这个文件是干什么的？】
 * 火车票系统把所有数据存在 SQLite 数据库里（一个 .db 文件）。
 * Database 类负责：打开数据库、创建表、插入初始演示数据。
 *
 * 【SQLite 是什么？】
 * 一种轻量级文件数据库，不需要安装 MySQL 那样的服务器。
 * 整个数据库就是一个文件，例如 data/ticket.db。
 *
 * 【Qt 在这里做什么？】
 * Qt 提供了 QSqlDatabase、QSqlQuery 等类，用来执行 SQL 语句。
 * 你仍然写的是标准 SQL（CREATE TABLE、INSERT、SELECT），只是通过 Qt 的 API 调用。
 *
 * 【其他成员怎么用？】
 * - 成员 C/D/E 的 Service 里写：Database::instance().connection()
 * - 拿到 QSqlDatabase 后，用 QSqlQuery 执行 SQL
 * - 界面层（成员 B）不要直接访问 Database，只调 Service
 */

#include <QSqlDatabase>
#include <QString>

class Database
{
public:
    /**
     * @brief 获取全局唯一的 Database 对象（单例模式）
     *
     * 【单例是什么？】
     * 整个程序只需要一个数据库连接，所以用 static 保证只创建一次。
     * 用法：Database::instance().initialize("data/ticket.db");
     */
    static Database &instance();

    /**
     * @brief 打开数据库文件，建表，并在首次运行时插入种子数据
     * @param dbFilePath 数据库文件路径，例如 "data/ticket.db"
     * @return 成功返回 true，失败返回 false（可通过 lastError() 看原因）
     */
    bool initialize(const QString &dbFilePath);

    /** @brief 返回已打开的数据库连接，供 QSqlQuery 使用 */
    QSqlDatabase connection() const;

    /** @brief 是否已成功 initialize */
    bool isOpen() const;

    /** @brief 最近一次失败的中文错误信息 */
    QString lastError() const;

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

private:
    Database() = default;

    /** @brief 执行 CREATE TABLE，创建所有业务表 */
    bool createTables();

    /** @brief 若表为空，插入演示用的站点、车次、用户等 */
    bool seedIfEmpty();

    /** @brief 设置错误信息并返回 false，方便统一错误处理 */
    bool fail(const QString &message);

    QString m_connectionName;  ///< Qt 要求每个连接有唯一名字，不能重复
    QString m_dbFilePath;      ///< 当前打开的数据库文件路径
    QString m_lastError;       ///< 最后一次错误描述
    bool m_open = false;       ///< 是否已成功打开
};
