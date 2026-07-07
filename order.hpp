#ifndef ORDER_HPP
#define ORDER_HPP
#include <string>
using namespace std;
/**********************/
struct order
{
    string id;
    string username;
    string train_number;
    string start_station;
    string end_station;
    string start_time;
    string end_time;
    string sear_type;
    string seat_number;
    int tickit_number;
    double price;
    string state;
    string order_time;
};
#endif