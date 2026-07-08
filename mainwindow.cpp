#include "mainwindow.h"
#include "TrainService.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(DataStore &store, QWidget *parent)
    : QMainWindow(parent)
    , m_store(store)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QSqlDatabase db = m_store.getDatabase();
    TrainService trainService(db);


}

MainWindow::~MainWindow()
{
    delete ui;
}
