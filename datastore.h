#ifndef DATASTORE_H
#define DATASTORE_H

#include <QString>
#include <QSqlDatabase>

class DataStore {
public:
    // 构造函数：默认在当前目录下创建 train_system.db 文件
    explicit DataStore(const QString &dbName = "train_system.db");
    ~DataStore();

    // 🌟 核心接口：允许 MainWindow 和 TrainService 获取这个 SQLite 连接
    QSqlDatabase getDatabase() const;

private:
    QSqlDatabase m_db;

    void initTables();     // 自动创建火车数据表
    void insertMockData(); // 自动预装初始车次数据
};

#endif // DATASTORE_H