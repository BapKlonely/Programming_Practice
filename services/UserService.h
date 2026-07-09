#pragma once
#include "models/user.h"
#include <QString>
#include <QVector>

class Database;

class UserService
{
public:
    explicit UserService(Database &db);
    ~UserService();

    bool setEnabled(int userId, bool enabled, QString *err);
    QVector<User> listConsumers();

private:
    Database &m_db;
};