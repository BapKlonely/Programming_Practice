#pragma once

#include "datastore.h"

#include <QDialog>
#include <QLineEdit>

class LoginDialog : public QDialog {
public:
    explicit LoginDialog(DataStore &store, QWidget *parent = nullptr);

    QString username() const;
    bool isAdmin() const;

private:
    DataStore &m_store;
    QString m_username;
    bool m_isAdmin = false;

    QLineEdit *m_loginUser = nullptr;
    QLineEdit *m_loginPass = nullptr;
    QLineEdit *m_registerUser = nullptr;
    QLineEdit *m_registerPass = nullptr;
    QLineEdit *m_registerName = nullptr;
    QLineEdit *m_registerPhone = nullptr;

    void login();
    void registerAccount();
};
