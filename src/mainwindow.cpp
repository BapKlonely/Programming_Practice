#include "mainwindow.h"

#include "customtitlebar.h"
#include "logindialog.h"

#include <QApplication>
#include <QBoxLayout>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTime>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTime>

#include <algorithm>
#include <functional>

struct RouteOption {
    QVector<Train> legs;
    double totalPrice = 0.0;
    int totalMinutes = 0;
    int leftSeats = 0;
};

static QTime parseTime(const QString &text)
{
    return QTime::fromString(text, "hh:mm");
}

static QDateTime departDateTime(const Train &train)
{
    return QDateTime(train.date, parseTime(train.departTime));
}

static QDateTime arriveDateTime(const Train &train)
{
    const QDateTime depart = departDateTime(train);
    QDate arriveDate = train.arriveDate.isValid() ? train.arriveDate : train.date;
    QDateTime arrive(arriveDate, parseTime(train.arriveTime));
    if (!train.arriveDate.isValid() && arrive <= depart) {
        arrive = arrive.addDays(1);
    }
    return arrive;
}

static int minutesBetween(const QDateTime &from, const QDateTime &to)
{
    return static_cast<int>(from.secsTo(to) / 60);
}

static QString formatMinutes(int minutes)
{
    return QString("%1小时%2分钟").arg(minutes / 60).arg(minutes % 60);
}

static QVector<RouteOption> buildRouteOptions(const QVector<Train> &trains,
                                              const QString &number,
                                              const QString &from,
                                              const QString &to,
                                              const QDate &date,
                                              const QString &sortMode,
                                              int maxTransfers)
{
    QVector<RouteOption> routes;
    if (from.trimmed().isEmpty() || to.trimmed().isEmpty()) {
        return routes;
    }

    const int maxLegs = qBound(1, maxTransfers + 1, 4);

    auto appendRoute = [&](const QVector<Train> &legs) {
        if (legs.isEmpty()) return;
        RouteOption option;
        option.legs = legs;
        option.leftSeats = legs.first().leftSeats;
        for (const auto &leg : legs) {
            option.totalPrice += leg.price;
            option.leftSeats = qMin(option.leftSeats, leg.leftSeats);
        }
        option.totalMinutes = minutesBetween(departDateTime(legs.first()), arriveDateTime(legs.last()));
        routes.push_back(option);
    };

    if (!number.isEmpty()) {
        for (const auto &train : trains) {
            if (!train.number.contains(number, Qt::CaseInsensitive)) continue;
            if (train.from != from || train.to != to) continue;
            if (date.isValid() && train.date != date) continue;
            appendRoute({train});
        }
    } else {
        std::function<void(const QString &, const QDateTime &, QVector<Train> &, QStringList &)> search;
        search = [&](const QString &currentCity, const QDateTime &earliestDepart, QVector<Train> &path, QStringList &visitedCities) {
            if (path.size() >= maxLegs || routes.size() >= 200) return;

            for (const auto &train : trains) {
                if (train.from != currentCity) continue;
                if (path.isEmpty() && date.isValid() && train.date != date) continue;
                if (visitedCities.contains(train.to)) continue;

                const QDateTime depart = departDateTime(train);
                if (!path.isEmpty()) {
                    if (depart < earliestDepart) continue;
                    if (minutesBetween(earliestDepart, depart) > 24 * 60) continue;
                }

                path.push_back(train);
                visitedCities << train.to;

                if (train.to == to) {
                    appendRoute(path);
                } else {
                    search(train.to, arriveDateTime(train), path, visitedCities);
                }

                visitedCities.removeLast();
                path.removeLast();
            }
        };

        QVector<Train> path;
        QStringList visitedCities{from};
        search(from, {}, path, visitedCities);
    }

    std::sort(routes.begin(), routes.end(), [&](const RouteOption &left, const RouteOption &right) {
        if (sortMode == "按价格排序") {
            return left.totalPrice < right.totalPrice;
        }
        if (sortMode == "按耗时排序") {
            return left.totalMinutes < right.totalMinutes;
        }
        return departDateTime(left.legs.first()) < departDateTime(right.legs.first());
    });
    return routes;
}

static QFrame *metricCard(const QString &number, const QString &label, QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName("metricCard");
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 14, 18, 14);

    auto *numberLabel = new QLabel(number, card);
    numberLabel->setObjectName("metricNumber");
    auto *textLabel = new QLabel(label, card);
    textLabel->setObjectName("metricLabel");

    layout->addWidget(numberLabel);
    layout->addWidget(textLabel);
    return card;
}

static QLabel *sectionTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName("sectionTitle");
    return label;
}

MainWindow::MainWindow(DataStore &store, const QString &username, bool isAdmin, QWidget *parent)
    : QMainWindow(parent), m_store(store), m_currentUser(username), m_isAdmin(isAdmin)
{
    setWindowTitle("火车票务系统 - 主菜单");
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(new CustomTitleBar("火车票务系统 - 主菜单", this, true, central));

    auto *content = new QWidget(central);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(18);

    auto *header = new QFrame(central);
    header->setObjectName("heroPanel");
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(24, 20, 24, 20);

    auto *titleBox = new QVBoxLayout();
    auto *title = new QLabel("火车票务系统", header);
    title->setObjectName("heroTitle");
    auto *subtitle = new QLabel("通过主菜单进入查询购票、订单退票和管理员维护窗口", header);
    subtitle->setObjectName("heroSubtitle");
    titleBox->addWidget(title);
    titleBox->addWidget(subtitle);

    auto *userBadge = new QLabel(QString("当前用户：%1%2").arg(m_currentUser, m_isAdmin ? "（管理员）" : ""), header);
    userBadge->setObjectName("badge");

    headerLayout->addLayout(titleBox);
    headerLayout->addStretch();
    headerLayout->addWidget(userBadge);

    auto *metrics = new QHBoxLayout();
    metrics->setSpacing(14);
    metrics->addWidget(metricCard(QString::number(m_store.trains().size()), "可售车次", central));
    metrics->addWidget(metricCard(QString::number(m_store.orders().size()), "系统订单", central));
    metrics->addWidget(metricCard(m_isAdmin ? "管理员" : "普通用户", "当前权限", central));

    auto *menuPanel = new QFrame(central);
    menuPanel->setObjectName("panel");
    auto *menuLayout = new QVBoxLayout(menuPanel);
    menuLayout->setContentsMargins(20, 18, 20, 20);
    menuLayout->setSpacing(14);
    menuLayout->addWidget(sectionTitle("功能入口", menuPanel));

    auto *buttonGrid = new QGridLayout();
    buttonGrid->setSpacing(14);
    auto *ticketButton = new QPushButton("车票查询与购票\n按日期和路线查询余票并下单", menuPanel);
    auto *ordersButton = new QPushButton("我的订单与退票\n查看个人订单并办理退票", menuPanel);
    auto *trainManageButton = new QPushButton("车次管理\n新增、修改、删除车次信息", menuPanel);
    auto *allOrdersButton = new QPushButton("全部订单\n查看所有用户的订单记录", menuPanel);
    auto *logoutButton = new QPushButton("退出登录", menuPanel);
    logoutButton->setObjectName("logoutButton");

    const QList<QPushButton *> menuButtons = {ticketButton, ordersButton, trainManageButton, allOrdersButton};
    for (auto *button : menuButtons) {
        button->setObjectName("menuButton");
        button->setMinimumHeight(94);
    }
    logoutButton->setMinimumHeight(48);

    trainManageButton->setEnabled(m_isAdmin);
    allOrdersButton->setEnabled(m_isAdmin);

    connect(ticketButton, &QPushButton::clicked, this, [this]() { openTicketDialog(); });
    connect(ordersButton, &QPushButton::clicked, this, [this]() { openMyOrdersDialog(); });
    connect(trainManageButton, &QPushButton::clicked, this, [this]() { openTrainManageDialog(); });
    connect(allOrdersButton, &QPushButton::clicked, this, [this]() { openAllOrdersDialog(); });
    connect(logoutButton, &QPushButton::clicked, this, [this]() { logout(); });

    buttonGrid->addWidget(ticketButton, 0, 0);
    buttonGrid->addWidget(ordersButton, 0, 1);
    buttonGrid->addWidget(trainManageButton, 1, 0);
    buttonGrid->addWidget(allOrdersButton, 1, 1);

    menuLayout->addLayout(buttonGrid);
    menuLayout->addWidget(logoutButton);

    layout->addWidget(header);
    layout->addLayout(metrics);
    layout->addWidget(menuPanel);
    rootLayout->addWidget(content);
    setCentralWidget(central);
}

void MainWindow::openTicketDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("车票查询与购票");
    dialog.setWindowFlags(dialog.windowFlags() | Qt::FramelessWindowHint);
    dialog.resize(1180, 700);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(14);
    layout->addWidget(new CustomTitleBar("车票查询与购票", &dialog, false, &dialog));

    auto *title = new QLabel("车票查询与购票", &dialog);
    title->setObjectName("pageTitle");
    auto *subtitle = new QLabel("选择车次后填写乘车人信息，系统会自动校验余票并生成订单", &dialog);
    subtitle->setObjectName("pageSubtitle");

    auto *filterBox = new QGroupBox("查询条件", &dialog);
    auto *filters = new QHBoxLayout(filterBox);
    auto *numberEdit = new QLineEdit(filterBox);
    numberEdit->setPlaceholderText("车次");
    auto *fromEdit = new QLineEdit(filterBox);
    fromEdit->setPlaceholderText("出发地");
    auto *toEdit = new QLineEdit(filterBox);
    toEdit->setPlaceholderText("目的地");
    auto *dateEdit = new QDateEdit(QDate::currentDate().addDays(1), filterBox);
    dateEdit->setCalendarPopup(true);
    auto *sortCombo = new QComboBox(filterBox);
    sortCombo->addItems({"默认排序", "按价格排序", "按耗时排序"});
    auto *transferCombo = new QComboBox(filterBox);
    transferCombo->addItems({"0", "1", "2", "3"});
    transferCombo->setCurrentText("2");
    auto *queryButton = new QPushButton("查询车票", filterBox);
    filters->addWidget(new QLabel("车次"));
    filters->addWidget(numberEdit);
    filters->addWidget(new QLabel("出发地"));
    filters->addWidget(fromEdit);
    filters->addWidget(new QLabel("目的地"));
    filters->addWidget(toEdit);
    filters->addWidget(new QLabel("日期"));
    filters->addWidget(dateEdit);
    filters->addWidget(new QLabel("排序"));
    filters->addWidget(sortCombo);
    filters->addWidget(new QLabel("最多中转"));
    filters->addWidget(transferCombo);
    filters->addWidget(queryButton);

    auto *table = new QTableWidget(&dialog);
    setTableHeaders(table, {"方案", "车次", "路线", "日期", "出发", "到达", "席别", "总票价", "总耗时", "可购票数"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto *bookBox = new QGroupBox("购票信息", &dialog);
    auto *bookLayout = new QHBoxLayout(bookBox);
    auto *nameEdit = new QLineEdit(bookBox);
    nameEdit->setPlaceholderText("乘车人姓名");
    auto *idEdit = new QLineEdit(bookBox);
    idEdit->setPlaceholderText("证件号");
    auto *countEdit = new QSpinBox(bookBox);
    countEdit->setRange(1, 10);
    auto *bookButton = new QPushButton("购买选中车次", bookBox);
    bookLayout->addWidget(new QLabel("乘车人"));
    bookLayout->addWidget(nameEdit);
    bookLayout->addWidget(new QLabel("证件号"));
    bookLayout->addWidget(idEdit);
    bookLayout->addWidget(new QLabel("张数"));
    bookLayout->addWidget(countEdit);
    bookLayout->addWidget(bookButton);

    auto fillRouteTable = [&]() {
        table->setRowCount(0);
        const QVector<RouteOption> routes = buildRouteOptions(m_store.trains(),
                                                              numberEdit->text().trimmed(),
                                                              fromEdit->text().trimmed(),
                                                              toEdit->text().trimmed(),
                                                              dateEdit->date(),
                                                              sortCombo->currentText(),
                                                              transferCombo->currentText().toInt());
        for (const auto &route : routes) {
            if (route.legs.isEmpty()) continue;
            const int row = table->rowCount();
            table->insertRow(row);

            QStringList trainNumbers;
            QStringList cities{route.legs.first().from};
            QStringList dates;
            QStringList seatTypes;
            for (const auto &leg : route.legs) {
                trainNumbers << leg.number;
                cities << leg.to;
                dates << QString("%1 -> %2")
                             .arg(leg.date.toString("yyyy-MM-dd"),
                                  arriveDateTime(leg).date().toString("yyyy-MM-dd"));
                seatTypes << leg.seatType;
            }

            const int transferCount = route.legs.size() - 1;
            const QString type = transferCount == 0 ? "直达" : QString("%1次中转").arg(transferCount);
            const QString depart = route.legs.first().departTime;
            const QString arrive = route.legs.last().arriveTime;

            const QStringList values = {
                type,
                trainNumbers.join(" / "),
                cities.join(" - "),
                dates.join(" / "),
                depart,
                arrive,
                seatTypes.join(" / "),
                QString::number(route.totalPrice, 'f', 2),
                formatMinutes(route.totalMinutes),
                QString::number(route.leftSeats)
            };

            for (int col = 0; col < values.size(); ++col) {
                auto *item = new QTableWidgetItem(values[col]);
                if (col == 0) {
                    item->setData(Qt::UserRole, trainNumbers);
                }
                table->setItem(row, col, item);
            }
        }
    };

    auto selectedRouteNumbers = [&]() {
        QStringList numbers;
        const auto selected = table->selectionModel()->selectedRows();
        if (selected.isEmpty()) return numbers;
        QTableWidgetItem *item = table->item(selected.first().row(), 0);
        if (!item) return numbers;
        numbers = item->data(Qt::UserRole).toStringList();
        return numbers;
    };

    connect(queryButton, &QPushButton::clicked, &dialog, [&]() {
        fillRouteTable();
    });
    connect(bookButton, &QPushButton::clicked, &dialog, [&]() {
        const QStringList numbers = selectedRouteNumbers();
        if (numbers.isEmpty()) {
            QMessageBox::warning(&dialog, "未选择方案", "请选择要购买的路线方案");
            return;
        }

        for (const auto &number : numbers) {
            const Train *train = m_store.findTrain(number);
            if (!train || train->leftSeats < countEdit->value()) {
                QMessageBox::warning(&dialog, "购票失败", "所选方案余票不足");
                return;
            }
        }

        QStringList orderIds;
        double amount = 0.0;
        for (const auto &number : numbers) {
            const Train *train = m_store.findTrain(number);
            Order order;
            QString error;
            if (!m_store.createOrder(m_currentUser, *train, nameEdit->text(), idEdit->text(), countEdit->value(), &order, &error)) {
                QMessageBox::warning(&dialog, "购票失败", error);
                return;
            }
            orderIds << order.id;
            amount += order.amount;
        }

        QMessageBox::information(&dialog,
                                 "购票成功",
                                 QString("订单号：%1\n金额：%2 元").arg(orderIds.join("，")).arg(amount, 0, 'f', 2));
        fillRouteTable();
    });
    connect(sortCombo, &QComboBox::currentTextChanged, &dialog, [&]() {
        if (!fromEdit->text().trimmed().isEmpty() && !toEdit->text().trimmed().isEmpty()) {
            fillRouteTable();
        }
    });
    connect(transferCombo, &QComboBox::currentTextChanged, &dialog, [&]() {
        if (!fromEdit->text().trimmed().isEmpty() && !toEdit->text().trimmed().isEmpty()) {
            fillRouteTable();
        }
    });

    connect(fromEdit, &QLineEdit::returnPressed, &dialog, fillRouteTable);
    connect(toEdit, &QLineEdit::returnPressed, &dialog, fillRouteTable);

    auto *hint = new QLabel("提示：只填写起点和终点可查询直达及多次中转方案；填写车次时仅查询匹配的直达车次。", &dialog);
    hint->setObjectName("mutedLabel");

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(filterBox);
    layout->addWidget(hint);
    layout->addWidget(table);
    layout->addWidget(bookBox);
    fillRouteTable();
    dialog.exec();
}

void MainWindow::openMyOrdersDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("我的订单与退票");
    dialog.setWindowFlags(dialog.windowFlags() | Qt::FramelessWindowHint);
    dialog.resize(1080, 600);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(14);
    layout->addWidget(new CustomTitleBar("我的订单与退票", &dialog, false, &dialog));
    auto *title = new QLabel("我的订单与退票", &dialog);
    title->setObjectName("pageTitle");
    auto *subtitle = new QLabel("选中已出票订单后可以办理退票，系统会自动回滚对应车次余票", &dialog);
    subtitle->setObjectName("pageSubtitle");

    auto *table = new QTableWidget(&dialog);
    setTableHeaders(table, {"订单号", "用户", "车次", "路线", "日期", "出发", "乘车人", "张数", "金额", "状态", "下单时间"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto *refundButton = new QPushButton("退选中订单", &dialog);
    refundButton->setObjectName("dangerButton");
    connect(refundButton, &QPushButton::clicked, &dialog, [&]() {
        const QString orderId = selectedValue(table);
        if (orderId.isEmpty()) {
            QMessageBox::warning(&dialog, "未选择订单", "请选择要退票的订单");
            return;
        }
        QString error;
        if (!m_store.refundOrder(orderId, &error)) {
            QMessageBox::warning(&dialog, "退票失败", error);
            return;
        }
        fillOrderTable(table, false);
    });

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(table);
    layout->addWidget(refundButton);
    fillOrderTable(table, false);
    dialog.exec();
}

void MainWindow::openTrainManageDialog()
{
    if (!m_isAdmin) return;

    QDialog dialog(this);
    dialog.setWindowTitle("车次管理");
    dialog.setWindowFlags(dialog.windowFlags() | Qt::FramelessWindowHint);
    dialog.resize(1140, 720);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(14);
    layout->addWidget(new CustomTitleBar("车次管理", &dialog, false, &dialog));
    auto *title = new QLabel("车次管理", &dialog);
    title->setObjectName("pageTitle");
    auto *subtitle = new QLabel("双击表格行可载入车次，修改后点击保存；删除会移除对应车次", &dialog);
    subtitle->setObjectName("pageSubtitle");

    auto *table = new QTableWidget(&dialog);
    setTableHeaders(table, {"车次", "出发地", "目的地", "出发日期", "出发", "到达日期", "到达", "席别", "票价", "总票", "余票"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto *formBox = new QGroupBox("车次信息", &dialog);
    auto *form = new QGridLayout(formBox);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(10);
    auto *numberEdit = new QLineEdit(formBox);
    auto *fromEdit = new QLineEdit(formBox);
    auto *toEdit = new QLineEdit(formBox);
    auto *dateEdit = new QDateEdit(QDate::currentDate().addDays(1), formBox);
    dateEdit->setCalendarPopup(true);
    auto *departEdit = new QLineEdit(formBox);
    auto *arriveDateEdit = new QDateEdit(QDate::currentDate().addDays(1), formBox);
    arriveDateEdit->setCalendarPopup(true);
    auto *arriveEdit = new QLineEdit(formBox);
    auto *seatTypeEdit = new QLineEdit(formBox);
    auto *priceEdit = new QDoubleSpinBox(formBox);
    priceEdit->setRange(0, 9999);
    priceEdit->setDecimals(2);
    auto *totalEdit = new QSpinBox(formBox);
    totalEdit->setRange(0, 9999);
    auto *leftEdit = new QSpinBox(formBox);
    leftEdit->setRange(0, 9999);
    auto *saveButton = new QPushButton("保存车次", formBox);
    auto *deleteButton = new QPushButton("删除车次", formBox);
    deleteButton->setObjectName("dangerButton");
    auto *clearButton = new QPushButton("清空", formBox);
    clearButton->setObjectName("secondaryButton");

    form->addWidget(new QLabel("车次"), 0, 0);
    form->addWidget(numberEdit, 0, 1);
    form->addWidget(new QLabel("出发地"), 0, 2);
    form->addWidget(fromEdit, 0, 3);
    form->addWidget(new QLabel("目的地"), 0, 4);
    form->addWidget(toEdit, 0, 5);
    form->addWidget(new QLabel("日期"), 1, 0);
    form->addWidget(dateEdit, 1, 1);
    form->addWidget(new QLabel("出发"), 1, 2);
    form->addWidget(departEdit, 1, 3);
    form->addWidget(new QLabel("到达日期"), 1, 4);
    form->addWidget(arriveDateEdit, 1, 5);
    form->addWidget(new QLabel("到达"), 2, 0);
    form->addWidget(arriveEdit, 2, 1);
    form->addWidget(new QLabel("席别"), 2, 2);
    form->addWidget(seatTypeEdit, 2, 3);
    form->addWidget(new QLabel("票价"), 2, 4);
    form->addWidget(priceEdit, 2, 5);
    form->addWidget(new QLabel("总票"), 3, 0);
    form->addWidget(totalEdit, 3, 1);
    form->addWidget(new QLabel("余票"), 3, 2);
    form->addWidget(leftEdit, 3, 3);
    form->addWidget(saveButton, 4, 3);
    form->addWidget(deleteButton, 4, 4);
    form->addWidget(clearButton, 4, 5);

    auto clearForm = [&]() {
        numberEdit->clear();
        fromEdit->clear();
        toEdit->clear();
        dateEdit->setDate(QDate::currentDate().addDays(1));
        departEdit->clear();
        arriveDateEdit->setDate(QDate::currentDate().addDays(1));
        arriveEdit->clear();
        seatTypeEdit->clear();
        priceEdit->setValue(0);
        totalEdit->setValue(0);
        leftEdit->setValue(0);
    };
    auto loadSelected = [&]() {
        const Train *train = m_store.findTrain(selectedValue(table));
        if (!train) return;
        numberEdit->setText(train->number);
        fromEdit->setText(train->from);
        toEdit->setText(train->to);
        dateEdit->setDate(train->date);
        departEdit->setText(train->departTime);
        arriveDateEdit->setDate(arriveDateTime(*train).date());
        arriveEdit->setText(train->arriveTime);
        seatTypeEdit->setText(train->seatType);
        priceEdit->setValue(train->price);
        totalEdit->setValue(train->totalSeats);
        leftEdit->setValue(train->leftSeats);
    };

    connect(table, &QTableWidget::cellDoubleClicked, &dialog, [=, &loadSelected](int, int) { loadSelected(); });
    connect(clearButton, &QPushButton::clicked, &dialog, clearForm);
    connect(saveButton, &QPushButton::clicked, &dialog, [&]() {
        Train train;
        train.number = numberEdit->text().trimmed();
        train.from = fromEdit->text().trimmed();
        train.to = toEdit->text().trimmed();
        train.date = dateEdit->date();
        train.departTime = departEdit->text().trimmed();
        train.arriveDate = arriveDateEdit->date();
        train.arriveTime = arriveEdit->text().trimmed();
        train.seatType = seatTypeEdit->text().trimmed();
        train.price = priceEdit->value();
        train.totalSeats = totalEdit->value();
        train.leftSeats = leftEdit->value();
        QString error;
        if (!m_store.upsertTrain(train, &error)) {
            QMessageBox::warning(&dialog, "保存失败", error);
            return;
        }
        fillTrainTable(table);
    });
    connect(deleteButton, &QPushButton::clicked, &dialog, [&]() {
        const QString number = numberEdit->text().trimmed().isEmpty() ? selectedValue(table) : numberEdit->text().trimmed();
        QString error;
        if (!m_store.removeTrain(number, &error)) {
            QMessageBox::warning(&dialog, "删除失败", error);
            return;
        }
        clearForm();
        fillTrainTable(table);
    });

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(table);
    layout->addWidget(formBox);
    fillTrainTable(table);
    dialog.exec();
}

void MainWindow::openAllOrdersDialog()
{
    if (!m_isAdmin) return;

    QDialog dialog(this);
    dialog.setWindowTitle("全部订单");
    dialog.setWindowFlags(dialog.windowFlags() | Qt::FramelessWindowHint);
    dialog.resize(1080, 600);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(14);
    layout->addWidget(new CustomTitleBar("全部订单", &dialog, false, &dialog));
    auto *title = new QLabel("全部订单", &dialog);
    title->setObjectName("pageTitle");
    auto *subtitle = new QLabel("管理员可以查看所有用户的购票和退票记录", &dialog);
    subtitle->setObjectName("pageSubtitle");
    auto *table = new QTableWidget(&dialog);
    setTableHeaders(table, {"订单号", "用户", "车次", "路线", "日期", "出发", "乘车人", "张数", "金额", "状态", "下单时间"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(table);
    fillOrderTable(table, true);
    dialog.exec();
}

void MainWindow::logout()
{
    close();

    LoginDialog login(m_store);
    if (login.exec() != QDialog::Accepted) {
        qApp->quit();
        return;
    }

    auto *window = new MainWindow(m_store, login.username(), login.isAdmin());
    window->resize(size());
    window->show();
}

void MainWindow::fillTrainTable(QTableWidget *table, const QString &number, const QString &from, const QString &to, const QDate &date) const
{
    table->setRowCount(0);
    for (const auto &train : m_store.trains()) {
        if (!number.isEmpty() && !train.number.contains(number, Qt::CaseInsensitive)) continue;
        if (!from.isEmpty() && !train.from.contains(from)) continue;
        if (!to.isEmpty() && !train.to.contains(to)) continue;
        if (date.isValid() && train.date != date) continue;

        const int row = table->rowCount();
        table->insertRow(row);
        const bool adminTable = table->columnCount() == 11;
        QStringList values = {
            train.number, train.from, train.to, train.date.toString("yyyy-MM-dd"),
            train.departTime
        };
        if (adminTable) values << arriveDateTime(train).date().toString("yyyy-MM-dd");
        values << train.arriveTime << train.seatType << QString::number(train.price, 'f', 2);
        if (adminTable) values << QString::number(train.totalSeats);
        values << QString::number(train.leftSeats);
        for (int col = 0; col < values.size(); ++col) {
            table->setItem(row, col, new QTableWidgetItem(values[col]));
        }
    }
}

void MainWindow::fillOrderTable(QTableWidget *table, bool allUsers) const
{
    table->setRowCount(0);
    for (const auto &order : m_store.orders()) {
        if (!allUsers && order.username != m_currentUser) continue;

        const int row = table->rowCount();
        table->insertRow(row);
        const QStringList values = {
            order.id, order.username, order.trainNumber, order.from + " - " + order.to,
            order.date.toString("yyyy-MM-dd"), order.departTime, order.passengerName,
            QString::number(order.count), QString::number(order.amount, 'f', 2),
            order.status, order.createdAt
        };
        for (int col = 0; col < values.size(); ++col) {
            table->setItem(row, col, new QTableWidgetItem(values[col]));
        }
    }
}

void MainWindow::setTableHeaders(QTableWidget *table, const QStringList &headers) const
{
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->verticalHeader()->setVisible(false);
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
}

QString MainWindow::selectedValue(QTableWidget *table, int column) const
{
    const auto selected = table->selectionModel()->selectedRows();
    if (selected.isEmpty() || !table->item(selected.first().row(), column)) {
        return {};
    }
    return table->item(selected.first().row(), column)->text();
}
