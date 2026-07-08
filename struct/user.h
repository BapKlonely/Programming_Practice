#ifndef USER_HPP
#define USER_HPP
#include <QString>
/*************************/
//通过枚举初步筛选用户类型//
enum class user_type
{
    guest=0,
    consumer=1,
    administrator=2
};
struct user_information
{
    QString account;//用户名
    QString password;//密码
    QString name;//姓名
    bool is_adminstrator;//是否为管理员(再次筛选)
    bool enabled_account;//账户是否启用
    user_type type;//用户类型
};
#endif