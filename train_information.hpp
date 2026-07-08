#ifndef TRAININ_FOTMATION_HPP
#define TRAININ_FOTMATION_HPP
#include <string>
using namespace std;
/******************************/
struct seat_information
{
    double price=0;
    int remaing_seat=0;
};
struct train_information
{
    string id;
    string start_station;
    string end_station;
    string start_time;
    string end_time;
    seat_information hard_seat;
    seat_information soft_seat;
    seat_information hard_sleep;
    seat_information soft_sleep;
};
#endif
