#include "datastore.h"
#include <QJsonArray>
#include <QJsonObject>   // 👈 1. 必须引入这个头文件！
#include <QJsonDocument>
#include <QDebug>

QString DataStore::datafile(const QString &path){
    return QDir(m_path).filePath(path);
}

DataStore::DataStore(const QString &path):m_path(path){
    qDebug() << "========================================";
    qDebug() << "🚀 确认：DataStore 构造函数真的开始运行了！";
    qDebug() << "📂 核心线索：代码试图写入的绝对路径是:" << QDir(m_path).absoluteFilePath("train.json");
    qDebug() << "========================================";

    QDir().mkpath(path);
    QFile ftrain(datafile("train.json")),fuser(datafile("user.json"));

    // =========================================================================
    // 1. 火车车次数据初始化
    // =========================================================================
    if(!ftrain.exists()){
        m_train={
            {"G114","北京","上海", QDate(2026,7,7), "15:30", QDate(2026,7,7), "18:30", "一等座", 200, 50, 550.0},
            {"D514","成都","西安", QDate(2026,7,7), "18:30", QDate(2026,7,8), "01:30", "二等座", 100, 80, 423.0},
            {"K1919","西安","北京", QDate(2026,7,8), "09:40", QDate(2026,7,9), "16:30", "硬卧", 90, 45, 320.0}
        };
        
        QJsonArray array;
        // 🛠️ 修复点：将 Train 结构体手动包装为 QJsonObject 塞入数组
        for(const Train& train : m_train) {
            QJsonObject obj;
            obj["trainId"]     = train.trainId;
            obj["destination"] = train.destination;
            obj["start"]       = train.start;
            obj["startDate"]   = train.startDate.toString("yyyy-MM-dd"); // QDate 需要转成 QString 存储
            obj["timeStart"]   = train.timeStart;
            obj["ArriveDate"]  = train.ArriveDate.toString("yyyy-MM-dd"); // QDate 需要转成 QString 存储
            obj["timeEnd"]     = train.timeEnd;
            obj["seatType"]    = train.seatType;
            obj["totalSeat"]   = train.totalSeat;
            obj["restSeat"]    = train.restSeat;
            obj["price"]       = train.price;
            
            array.push_back(obj); // 👈 现在塞入的是 QJsonObject，Qt 欣然接受！
        }
        if (ftrain.open(QFile::WriteOnly)) {
            ftrain.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
            ftrain.close(); // 写完记得关门
        }
    }

    // =========================================================================
    // 2. 用户数据初始化
    // =========================================================================
    if(!fuser.exists()){
        m_user={
            {{},"admin","admin123","admin","10086",true}
        };
        
        QJsonArray array;
        // 🛠️ 修复点：将 User 结构体（包含嵌套的 Order 数组）包装为 QJsonObject
        for(const User& user : m_user){
            QJsonObject userObj;
            userObj["username"] = user.username;
            userObj["password"] = user.password;
            userObj["name"]     = user.name;
            userObj["phone"]    = user.phone;
            userObj["isAdmin"]  = user.isAdmin;
            
            // 核心：处理 User 结构体内部嵌套的 std::vector<Order> 历史订单
            QJsonArray orderArray;
            for(const Order& order : user.orders) {
                QJsonObject orderObj;
                orderObj["id"]        = order.id;
                orderObj["username"]  = order.username;
                orderObj["phone"]     = order.phone;
                orderObj["train"]     = order.train;
                orderObj["createdAt"] = order.createdAt;
                orderObj["count"]     = order.count;
                orderArray.push_back(orderObj);
            }
            userObj["orders"] = orderArray; // 把订单数组嵌套进用户对象中
            
            array.push_back(userObj); // 👈 成功塞入
        }
        if (fuser.open(QFile::WriteOnly)) {
            fuser.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
            fuser.close(); // 写完记得关门
        }
    }
}