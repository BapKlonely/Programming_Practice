#pragma once
#include "models/user.h"
#include <QString>
#include <QVector>
#include <optional>

class Database;
class AuthService
{
public:
    explicit AuthService(Database &db);
    ~AuthService();
    QString getError() const;
    void loginAsGuest();
    std::optional<User> login(const QString &username,const QString &password);
    void logout();
    const User* getCurrentUser() const;
    bool isLoggedIn() const;
    bool isAdministrator() const;
    //判断用户类型
    static bool isAdmin(const User &user);
    static bool isConsumer(const User &user);
    static bool isGuest(const User &user);
    //修改密码
    bool changePassword(int userId,const QString &oldPwd,const QString &newPwd,QString *err);
    //注册账号
    bool createConsumer(const QString &username,const QString &password,QString *err);
    //查询
    const User* getUserByUsername(const QString &username) const;
private:
    void createDefaultUsers();
    User m_currentUser;
    QString m_error;
    bool m_loggedIn=false;
    Database &m_db;
};
