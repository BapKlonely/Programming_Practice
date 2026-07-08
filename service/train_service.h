#ifndef TRAIN_SERVICE_HPP
#define TRAIN_SERVICE_HPP
#include "struct/train.h"
#include <QString>
#include <QList>
/**********************/
class train_service//纯虚类
{
    public:
    virtual ~train_service()=default;
    virtual QList<train_information>get_train(const QString& start_station,const QString& end_station,const QString& start_day)=0;
    virtual bool search_train(const QString& train_id,train_information& out)=0;
    virtual QList<train_information>get_all_train()=0;
};
#endif