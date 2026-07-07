#ifndef TRAIN_H
#define TRAIN_H

#include <QString>
#include <QDate>

struct Train {
    int id;                 
    QString trainNo;        
    QString startStation;   
    QString endStation;     
    QString status;         // 严格限定为小写 "running" 或 "suspended"
};

struct TrainTicketInfo {
    QString trainNo;        
    QString fromStation;    
    QString toStation;      
    QDate date;             
    QString seatType;       // 严格限定为 "二等座"、"一等座" 或 "硬座"
    int availableSeats;     
    QString status;         
};

#endif // TRAIN_H