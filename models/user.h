#pragma once

/**
 * @file user.h
 * @brief 用户相关的数据结构（成员 C 主维护，全队 include）
 *
 * 【为什么单独放一个 .h？】
 * 这是“数据模型”，描述一条用户记录有哪些字段。
 * Service 和 UI 都通过 struct User 传递数据，而不是各自定义一套。
 *
 * 【和数据库表 users 的对应关系】
 * 表里的每一列，在这里对应 struct 里的一个成员变量。
 */

#include <QString>

struct User
{
    int id = 0;                 ///< 主键，数据库自动生成
    QString username;           ///< 登录名，唯一
    QString passwordHash;       ///< 密码哈希（成员 C 会用 SHA256+盐，现在可先存占位）
    QString salt;               ///< 密码盐值
    QString role;               ///< "admin" 管理员 或 "seller" 售票员
    bool enabled = true;        ///< false 表示账号被禁用，不能登录
    QString createdAt;          ///< 创建时间，字符串格式 yyyy-MM-dd hh:mm:ss

    /** @brief 是否是管理员，方便 UI 判断显示哪些按钮 */
    bool isAdmin() const { return role == QStringLiteral("admin"); }

    /** @brief 是否是售票员 */
    bool isSeller() const { return role == QStringLiteral("seller"); }
};
