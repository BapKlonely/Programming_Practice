#pragma once

/*
头文件使用说明：
描述了用户相关的类/结构体，包括用户名、密码（包含盐值加密）、身份等。
*/

#include <QString>

struct User
{
    int id = 0;                 // 用户ID，依赖数据库自增主键自动生成
    QString username;           // 用户登录名，唯一
    QString passwordHash;       // 密码哈希，当前可暂时用明文，后期再搞加密算法
    QString salt;               // 密码盐值
    QString role;               // "admin"表示管理员兼售票员，否则为游客
    bool enabled = true;        // false 表示账号被禁用，不能登录
    QString createdAt;          // 创建时间，字符串格式 yyyy-MM-dd hh:mm:ss

    //判断是否是管理，若是管理进入管理员界面
    bool isAdmin() const { return role == QStringLiteral("admin"); }
};
