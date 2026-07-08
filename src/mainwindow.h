#pragma once

#include "datastore.h"

#include <QMainWindow>
#include <QTableWidget>

class MainWindow : public QMainWindow {
public:
    MainWindow(DataStore &store, const QString &username, bool isAdmin, QWidget *parent = nullptr);

private:
    DataStore &m_store;
    QString m_currentUser;
    bool m_isAdmin = false;

    void openTicketDialog();
    void openMyOrdersDialog();
    void openTrainManageDialog();
    void openAllOrdersDialog();
    void logout();

    void fillTrainTable(QTableWidget *table,
                        const QString &number = {},
                        const QString &from = {},
                        const QString &to = {},
                        const QDate &date = {}) const;
    void fillOrderTable(QTableWidget *table, bool allUsers) const;
    void setTableHeaders(QTableWidget *table, const QStringList &headers) const;
    QString selectedValue(QTableWidget *table, int column = 0) const;
};
