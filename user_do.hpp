#ifndef USERDO_HPP
#define USERDO_HPP
#include "user.hpp"
#include <string>
#include <vector>
using namespace std;
/**************************/
class userdo
{
    private:
    vector<user> users;
    string datapath;
    string error;
    bool loadfromfile();
    bool savetofile();
    void defaultuser();
    public:
    userdo();//构造
    ~userdo();//析构
    string geterror() const//获取最后的错误信息
    {
        return error;
    }
    bool newuser(const user& username,string& error);//创建新用户
    user* checklogin(const string& username,const string& password,string& error);//登录检测
    bool changepassword(bool is_adminstrator,const string& username,const string& old_one,const string& new_one,string& error);//更换密码
    bool setuserenabled(bool is_adminstrator,const string& username,bool enabled,string& error);//设置用户状态
    vector<user> getlist(string& error);//获取所有成员
};
#endif