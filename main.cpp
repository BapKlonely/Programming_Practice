#include "mainwindow.h"
#include "datastore.h"
#include <QApplication>
#include <QDir>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 🌟 终极大招：直接获取当前终端所在的绝对路径，稳稳地在 build 目录下创建数据库
    QString dbPath = QDir::currentPath() + "/train_system.db";
    
    // 📢 把路径打印出来，让我们在终端里一眼看清它到底在哪落地
    qDebug() << "📢 正在请求 SQLite 降落，绝对路径为:" << dbPath;
    
    DataStore store(dbPath); 
    MainWindow w(store); 
    
    w.show();
    return a.exec();
}