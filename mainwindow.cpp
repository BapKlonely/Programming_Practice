#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(DataStore &store, QWidget *parent)
    : QMainWindow(parent)
    , m_store(store)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
