#ifndef TRAIN_SERVICE_HPP
#define TRAIN_SERVICE_HPP
#include "train_information.hpp"
#include <string>
#include <vector>
using namespace std;
/**********************/
class train_service
{
    public:
    virtual ~train_service()=default;
    virtual vector<train_information>get_train(const string& start_station,const string& end_station,const string &start_time)=0;
    virtual bool search_train(const string& train_id,train_information& out)=0;
};
#endif