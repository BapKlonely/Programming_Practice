#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(DataStore &store, QWidget *parent) 
    : QDialog(parent), m_store(store)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}
