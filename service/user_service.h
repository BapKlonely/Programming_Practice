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
    void loginAsGuest();//游客登录
    User* login(const QString& username,const QString& password,QString& error);//账号密码登录
    void logout();//登出
    const User* getCurrentUser() const;//获取当前用户
    bool isLoggedIn() const;//是否已登录
    bool isAdministrator() const;//是否管理员（当前会话）
    //判断用户类型
    static bool isAdmin(const User& user);
    static bool isConsumer(const User& user);
    static bool isGuest(const User& user);
    //用户管理
    bool registerUser(const User& user,QString& error);//注册新用户
    bool changePassword(bool isAdmin,const QString& username,const QString& oldPassword,const QString& newPassword,QString& error);//修改密码
    bool setUserEnabled(bool isAdmin,const QString& username,bool enabled,QString& error);//启用/禁用用户
    QList<User> getUserList(QString& error);//获取所有用户（管理员）
    //查询
    const User* getUserByUsername(const QString& username) const;//按用户名查找
private:
    User m_currentUser;//当前登录用户
    QString m_error;//错误信息
    bool m_loggedIn=false;//登录状态
    void createDefaultUsers();//创建初始管理员和消费者
};
