#include "mainwindow.h"
#include "datastore.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 👈 1. 核心：在这里真正创建大管家！传入 "data" 代表把 json 文件存在 data 文件夹下
    DataStore store("data"); 
    
    // 👈 2. 核心：把大管家 store 传给主窗口 w
    MainWindow w(store); 
    
    w.show();
    return QCoreApplication::exec();
}