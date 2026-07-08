#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include "datastore.h"

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(DataStore &store, QWidget *parent = nullptr);
    ~Dialog();

private:
    Ui::Dialog *ui;
    DataStore &m_store;
};

#endif // DIALOG_H
