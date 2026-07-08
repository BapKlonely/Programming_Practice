#ifndef USER_SERVICE_H
#define USER_SERVICE_H
#include "struct/user.h"
#include <QString>
#include <QList>
/**************************/
class user_service
{
    private:
    /*******************************/
    //文件操作//
    QList<user_information> users;//内存中的用户表
    QString user_file_path;//用户文件路径*********
    bool loadfromfile();//加载文件
    bool savetofile();//写入文件
    /******************************/
    void defaultuser();//创建初始管理员
    QString error;
    public:
    user_service();//构造
    ~user_service();//析构
    QString geterror() const//获取最后的错误信息
    {
        return error;
    }
    /******************************/
    //判别用户类型//
    void login_as_guest();//以游客身份登录
    void get_current_user() const;//获取当前登录用户
    bool is_logged() const;//是否已登录
    bool is_adminstrator() const;//是否为管理员
    /******************************/
    bool newuser(const user_information& account, QString& error);//创建新用户（若成功，则返回TURE;反之，则返回FALSE）
    user_information* checklogin(const QString& account, const QString& password, QString& error);//登录检测
    bool changepassword(bool is_adminstrator, const QString& account, const QString& old_one, const QString& new_one, QString& error);//更换密码
    bool setuserenabled(bool is_adminstrator, const QString& account, bool enabled, QString& error);//设置用户状态
    QList<user_information> getlist(QString& error);//获取所有成员（管理员功能）
};
#endif
