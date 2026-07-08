#pragma once

/**
 * @file OrderService.h
 * @brief 订单查询服务（成员 E 负责，第 3 步实现）
 */

#include "models/order.h"

#include <QVector>
#include <optional>

class Database;

class OrderService
{
public:
    explicit OrderService(Database &db);

    std::optional<Order> getOrderById(const QString &orderNo);
    QVector<Order> searchOrdersByPassenger(const QString &name);

private:
    Database &m_db;
};
