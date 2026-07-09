#include "models/user.h"
#include <QString>
#include <QList>

class UserService
{
public:
    UserService();
    ~UserService();
    //错误信息
    QString getError() const
    {
        return m_error;
    }
    //会话管理
    void loginAsGuest();
    User* login(const QString& username,const QString& password,QString& error);
    void logout();
    const User* getCurrentUser() const;
    bool isLoggedIn() const;
    bool isAdministrator() const;
    //判断用户类型
    static bool isAdmin(const User& user);
    static bool isConsumer(const User& user);
    static bool isGuest(const User& user);
    //用户管理
    bool registerUser(const User& user,QString& error);
    bool changePassword(bool isAdmin,const QString& username,const QString& oldPassword,const QString& newPassword,QString& error);
    bool setUserEnabled(bool isAdmin,const QString& username,bool enabled,QString& error);
    QList<User> getUserList(QString& error);
    //查询
    const User* getUserByUsername(const QString& username) const;
private:
    User m_currentUser;
    QString m_error;
    bool m_loggedIn=false;
    void createDefaultUsers();
};
