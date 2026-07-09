#include <QApplication>
#include <QMessageBox>

#include "core/database.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("TrainTicketSystem"));

    // 初始化数据库
    Database &db = Database::instance();
    if (!db.initialize("data/ticket.db")) {
        QMessageBox::critical(nullptr, QStringLiteral("数据库错误"),
            QStringLiteral("无法初始化数据库：\n%1").arg(db.lastError()));
        return -1;
    }

    MainWindow window;
    window.show();

    return app.exec();
}
