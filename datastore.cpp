#include "datastore.h"
#include <QJsonArray>
#include <QJsonDocument>
QString DataStore::datafile(QString path){
    return QDir(m_path).filePath(path);
}

DataStore::DataStore(const QString &path):m_path(path){
    QDir().mkpath(path);
    QFile ftrain(datafile("train.json")),fuser(datafile("user.json"));
    // class Train{
    //     QString trainId;
    //     QString destination;
    //     QString start;
    //     QDate startDate;
    //     QString timeStart;
    //     QDate ArriveDate;
    //     QString timeEnd;
    //     QString seatType;
    //     int totalSeat;
    //     int restSeat;
    //     double price;
    // };
    if(!ftrain.exists()){
        m_train={
            {"G114","北京","上海","2026-7-7","15:30","2026-7-7","18:30","一等座","200","50","550.0"},
            {"D514","成都","西安","2026-7-7","18:30","2026-7-8","01:30","二等座","100","80","423.0"},
            {"K1919","西安","北京","2026-7-8","09:40","2026-7-9","16:30","硬卧","90","45","320"}
        };
            QJsonArray array;
        for(const Train&train:m_train)
            array.push_back(train);
        ftrain.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    }
    // class User{
    //     std::vector<Order> orders;
    //     QString username;
    //     QString password;
    //     QString name;
    //     QString phone;
    //     bool isAdmin=false;
    // };
    if(!fuser.exists()){
        m_user={
            {{},"admin","admin123","admin","10086",true}
        };
        QJsonArray array;
        for(const User&user:m_user){
            array.push_back(user);
        }
        fuser.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    }
}