#ifndef USER_H
#define USER_H
#include <QString>
/*************************/
//通过枚举初步筛选用户类型//
enum class user_type
{
    guest=0,
    consumer=1,
    administrator=2
};
//游客（仅能查看车次信息）：0
//乘客（拥有账号，可以查看车次信息，可以订票，退票等操作）：1
//管理员（拥有账号，可以查看车次信息，可以订票，退票等操作，可以管理用户信息）：2
/*************************/
struct user_information
{
    QString account;//用户名
    QString password;//密码
    QString name;//姓名
    bool is_adminstrator;//是否为管理员(再次筛选)
    bool enabled_account;//账户是否启用
    user_type type;//用户类型
};
//给出用户的账号，密码，姓名，是否为管理员，账户是否启用，用户类型
#endif
