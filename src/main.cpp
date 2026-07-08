#include "datastore.h"
#include "logindialog.h"
#include "mainwindow.h"
#include "style.h"

#include <QApplication>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("火车票务系统");
    QApplication::setStyle("Fusion");
    app.setStyleSheet(appStyleSheet());

    DataStore store(QDir(QApplication::applicationDirPath()).filePath("data"));
    store.load();

    LoginDialog login(store);
    if (login.exec() != QDialog::Accepted) {
        return 0;
    }

    MainWindow window(store, login.username(), login.isAdmin());
    window.resize(880, 560);
    window.show();

    return app.exec();
}
