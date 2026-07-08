/**
 * @file database_manual.cpp
 * @brief 数据库初始化测试程序（成员 E 独立调试，不依赖 UI）
 *
 * 【如何运行】
 * 1. 用 Qt Creator 打开 src/CMakeLists.txt，构建目标 ticket_test
 * 2. 或在 build 目录命令行：cmake --build . --target ticket_test
 * 3. 运行 ticket_test.exe，控制台会打印用户、车次、余票
 *
 * 【为什么需要 QCoreApplication？】
 * Qt 的 SQL、文件等模块要求程序里有一个 QCoreApplication 对象（哪怕没有窗口）。
 * 有界面时用 QApplication；纯控制台测试用 QCoreApplication 即可。
 *
 * 【测试通过后说明什么？】
 * - Database::initialize() 能创建 data/test_ticket.db
 * - 7 张表已建好
 * - 种子数据（2 用户、6 站点、6 车次）已写入
 */

#include "core/database.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

/** @brief 打印查询结果的前几列，方便肉眼检查 */
static void printQuery(const QString &title, const QString &sql)
{
    qDebug().noquote() << "\n==========" << title << "==========";

    QSqlQuery query(Database::instance().connection());
    if (!query.exec(sql)) {
        qDebug() << "查询失败:" << query.lastError().text();
        return;
    }

    const QSqlRecord rec = query.record();
    const int cols = rec.count();

    // 打印表头
    QString header;
    for (int c = 0; c < cols; ++c) {
        if (c > 0) header += " | ";
        header += rec.fieldName(c);
    }
    qDebug().noquote() << header;
    qDebug().noquote() << QString(cols * 12, QChar('-'));

    // 打印每一行
    int rows = 0;
    while (query.next()) {
        QString line;
        for (int c = 0; c < cols; ++c) {
            if (c > 0) line += " | ";
            line += query.value(c).toString();
        }
        qDebug().noquote() << line;
        ++rows;
    }
    qDebug() << "共" << rows << "行";
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // 数据库文件放在可执行文件同级的 data 目录下
    const QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("data/test_ticket.db"));
    qDebug() << "数据库路径:" << dbPath;

    Database &db = Database::instance();
    if (!db.initialize(dbPath)) {
        qDebug() << "初始化失败:" << db.lastError();
        return 1;
    }

    qDebug() << "初始化成功！";

    printQuery(QStringLiteral("用户表 users"), QStringLiteral("SELECT id, username, role, enabled FROM users"));
    printQuery(QStringLiteral("站点表 stations"), QStringLiteral("SELECT id, name FROM stations"));
    printQuery(QStringLiteral("车次表 trains"),
               QStringLiteral("SELECT id, train_no, travel_date, base_price, status FROM trains"));
    printQuery(QStringLiteral("席位库存 seat_inventory"),
               QStringLiteral("SELECT id, train_id, seat_type, total, sold, (total - sold) AS remaining FROM seat_inventory"));

    // 重点：T999 应只有 1 张余票，后续 TicketService 用来测超售
    printQuery(QStringLiteral("T999 余票（应为 1）"),
               QStringLiteral("SELECT train_no, seat_type, total - sold AS remaining "
                              "FROM seat_inventory si "
                              "JOIN trains t ON t.id = si.train_id "
                              "WHERE t.train_no = 'T999'"));

    qDebug() << "\n全部检查完成。若上方数据正常，第 1、2 步已成功。";
    return 0;
}
