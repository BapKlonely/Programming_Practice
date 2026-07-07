#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr,DataStore store);
    ~Dialog();

private:
    Ui::Dialog *ui;
    DataStore &m_store;
};

#endif // DIALOG_H
