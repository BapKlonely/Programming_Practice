#ifndef USER_HPP
#define USER_HPP
#include <string>
using namespace std;
/*************************/
struct user
{
    string account;
    string password;
    bool is_adminstrator;
    bool enabled_account;
};
#endif