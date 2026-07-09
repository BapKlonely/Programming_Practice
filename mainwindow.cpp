#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dialog.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_db(Database::instance())
    , m_auth(m_db)
    , m_trains(m_db)
    , m_users(m_db)
    , m_stack(nullptr)
    , m_tabs(nullptr)
{
    ui->setupUi(this);
    setupUI();
    setupMenuBar();
    updateStatusBar();
    updateUIForRole();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ==================== 菜单栏 ====================

void MainWindow::setupMenuBar()
{
    // ---- 账户菜单 ----
    QMenu *accountMenu = ui->menubar->addMenu(QStringLiteral("账户"));

    m_loginAction = accountMenu->addAction(QStringLiteral("登录"));
    connect(m_loginAction, &QAction::triggered, this, &MainWindow::onLogin);

    m_registerAction = accountMenu->addAction(QStringLiteral("注册"));
    connect(m_registerAction, &QAction::triggered, this, &MainWindow::onRegister);

    m_guestAction = accountMenu->addAction(QStringLiteral("游客模式"));
    connect(m_guestAction, &QAction::triggered, this, &MainWindow::onGuestMode);

    accountMenu->addSeparator();

    m_logoutAction = accountMenu->addAction(QStringLiteral("登出"));
    m_logoutAction->setEnabled(false);
    connect(m_logoutAction, &QAction::triggered, this, &MainWindow::onLogout);

    accountMenu->addSeparator();

    QAction *exitAction = accountMenu->addAction(QStringLiteral("退出"));
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);

    // ---- 功能菜单 ----
    QMenu *funcMenu = ui->menubar->addMenu(QStringLiteral("功能"));

    QAction *trainAction = funcMenu->addAction(QStringLiteral("列车查询"));
    trainAction->setEnabled(false);
    connect(trainAction, &QAction::triggered, this, [this]() {
        m_stack->setCurrentIndex(1);
        m_tabs->setCurrentIndex(0);
    });

    QAction *userAction = funcMenu->addAction(QStringLiteral("用户管理"));
    userAction->setEnabled(false);
    connect(userAction, &QAction::triggered, this, [this]() {
        m_stack->setCurrentIndex(1);
        m_tabs->setCurrentIndex(1);
    });

    // 登录状态变化时更新菜单项
    connect(m_loginAction, &QAction::triggered, this, [=]() {
        trainAction->setEnabled(m_auth.isLoggedIn());
        userAction->setEnabled(m_auth.isAdministrator());
    });
    connect(m_registerAction, &QAction::triggered, this, [=]() {
        trainAction->setEnabled(m_auth.isLoggedIn());
    });
    connect(m_guestAction, &QAction::triggered, this, [=]() {
        trainAction->setEnabled(true);
    });
    connect(m_logoutAction, &QAction::triggered, this, [=]() {
        trainAction->setEnabled(false);
        userAction->setEnabled(false);
    });
}

// ==================== 主界面布局 ====================

void MainWindow::setupUI()
{
    m_stack = new QStackedWidget(ui->centralwidget);

    // 页面0：欢迎页
    QLabel *welcomeLabel = new QLabel(QStringLiteral("欢迎使用火车票管理系统\n\n请通过「账户」菜单登录、注册或进入游客模式"));
    welcomeLabel->setAlignment(Qt::AlignCenter);
    welcomeLabel->setStyleSheet("font-size: 18px; color: #666;");
    m_stack->addWidget(welcomeLabel);

    // 页面1：功能标签页
    m_tabs = new QTabWidget;
    m_tabs->addTab(createTrainSearchPage(), QStringLiteral("列车查询"));
    m_tabs->addTab(createUserManagementPage(), QStringLiteral("用户管理"));
    m_stack->addWidget(m_tabs);

    QVBoxLayout *layout = new QVBoxLayout(ui->centralwidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stack);
}

// ==================== 列车查询页 ====================

QWidget* MainWindow::createTrainSearchPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout(page);

    // ---- 车次搜索 ----
    QGroupBox *searchGroup = new QGroupBox(QStringLiteral("车次搜索"));
    QHBoxLayout *searchLayout = new QHBoxLayout(searchGroup);

    searchLayout->addWidget(new QLabel(QStringLiteral("车次号：")));
    m_trainNoEdit = new QLineEdit;
    m_trainNoEdit->setPlaceholderText(QStringLiteral("如 G101，留空则模糊搜索"));
    searchLayout->addWidget(m_trainNoEdit);

    searchLayout->addWidget(new QLabel(QStringLiteral("出发站：")));
    m_fromStationEdit = new QLineEdit;
    m_fromStationEdit->setPlaceholderText(QStringLiteral("如 上海"));
    searchLayout->addWidget(m_fromStationEdit);

    searchLayout->addWidget(new QLabel(QStringLiteral("日期：")));
    m_dateEdit = new QDateEdit(QDate::currentDate());
    m_dateEdit->setCalendarPopup(true);
    searchLayout->addWidget(m_dateEdit);

    m_searchBtn = new QPushButton(QStringLiteral("搜索车次"));
    searchLayout->addWidget(m_searchBtn);
    connect(m_searchBtn, &QPushButton::clicked, this, &MainWindow::onSearchTrains);

    mainLayout->addWidget(searchGroup);

    // 车次结果表
    m_trainTable = new QTableWidget;
    m_trainTable->setColumnCount(7);
    m_trainTable->setHorizontalHeaderLabels({
        QStringLiteral("车次号"), QStringLiteral("出发站"), QStringLiteral("到达站"),
        QStringLiteral("出发时间"), QStringLiteral("到达时间"), QStringLiteral("基准票价"), QStringLiteral("状态")
    });
    m_trainTable->horizontalHeader()->setStretchLastSection(true);
    m_trainTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_trainTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_trainTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(m_trainTable);

    // ---- 余票查询 ----
    QGroupBox *ticketGroup = new QGroupBox(QStringLiteral("余票查询"));
    QHBoxLayout *ticketLayout = new QHBoxLayout(ticketGroup);

    ticketLayout->addWidget(new QLabel(QStringLiteral("出发站：")));
    m_ticketFromEdit = new QLineEdit;
    m_ticketFromEdit->setPlaceholderText(QStringLiteral("如 北京"));
    ticketLayout->addWidget(m_ticketFromEdit);

    ticketLayout->addWidget(new QLabel(QStringLiteral("到达站：")));
    m_ticketToEdit = new QLineEdit;
    m_ticketToEdit->setPlaceholderText(QStringLiteral("如 上海"));
    ticketLayout->addWidget(m_ticketToEdit);

    ticketLayout->addWidget(new QLabel(QStringLiteral("日期：")));
    m_ticketDateEdit = new QDateEdit(QDate::currentDate());
    m_ticketDateEdit->setCalendarPopup(true);
    ticketLayout->addWidget(m_ticketDateEdit);

    m_queryTicketBtn = new QPushButton(QStringLiteral("查询余票"));
    ticketLayout->addWidget(m_queryTicketBtn);
    connect(m_queryTicketBtn, &QPushButton::clicked, this, &MainWindow::onQueryTickets);

    mainLayout->addWidget(ticketGroup);

    // 余票结果表
    m_ticketTable = new QTableWidget;
    m_ticketTable->setColumnCount(9);
    m_ticketTable->setHorizontalHeaderLabels({
        QStringLiteral("车次号"), QStringLiteral("出发站"), QStringLiteral("到达站"),
        QStringLiteral("日期"), QStringLiteral("出发时间"), QStringLiteral("到达时间"),
        QStringLiteral("座位类型"), QStringLiteral("票价"), QStringLiteral("余票")
    });
    m_ticketTable->horizontalHeader()->setStretchLastSection(true);
    m_ticketTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ticketTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ticketTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(m_ticketTable);

    return page;
}

// ==================== 用户管理页（管理员） ====================

QWidget* MainWindow::createUserManagementPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);

    QLabel *title = new QLabel(QStringLiteral("消费者用户管理"));
    title->setStyleSheet("font-size: 16px; font-weight: bold;");
    layout->addWidget(title);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    m_refreshUsersBtn = new QPushButton(QStringLiteral("刷新列表"));
    connect(m_refreshUsersBtn, &QPushButton::clicked, this, &MainWindow::onRefreshUsers);
    btnLayout->addWidget(m_refreshUsersBtn);

    m_toggleUserBtn = new QPushButton(QStringLiteral("启用/禁用选中用户"));
    connect(m_toggleUserBtn, &QPushButton::clicked, this, &MainWindow::onToggleUserEnabled);
    btnLayout->addWidget(m_toggleUserBtn);

    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_userTable = new QTableWidget;
    m_userTable->setColumnCount(5);
    m_userTable->setHorizontalHeaderLabels({
        QStringLiteral("ID"), QStringLiteral("用户名"), QStringLiteral("角色"),
        QStringLiteral("状态"), QStringLiteral("创建时间")
    });
    m_userTable->horizontalHeader()->setStretchLastSection(true);
    m_userTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_userTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_userTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_userTable);

    return page;
}

// ==================== 状态栏 ====================

void MainWindow::updateStatusBar()
{
    const User *user = m_auth.getCurrentUser();
    if (m_auth.isLoggedIn() && user) {
        QString roleText = user->isAdmin() ? QStringLiteral("管理员") : QStringLiteral("消费者");
        ui->statusbar->showMessage(
            QStringLiteral("当前用户: %1 [%2]").arg(user->username, roleText));
    } else {
        ui->statusbar->showMessage(QStringLiteral("未登录"));
    }
}

// ==================== 角色控制 ====================

void MainWindow::updateUIForRole()
{
    bool loggedIn = m_auth.isLoggedIn();
    bool isAdmin = m_auth.isAdministrator();

    m_loginAction->setEnabled(!loggedIn);
    m_registerAction->setEnabled(!loggedIn);
    m_guestAction->setEnabled(!loggedIn);
    m_logoutAction->setEnabled(loggedIn);

    if (!loggedIn) {
        m_stack->setCurrentIndex(0); // 欢迎页
        m_tabs->setTabEnabled(1, false); // 隐藏用户管理
    } else {
        m_stack->setCurrentIndex(1); // 功能页
        m_tabs->setCurrentIndex(0);
        // 管理员才能看到用户管理标签
        m_tabs->setTabEnabled(1, isAdmin);
    }
}

// ==================== 槽函数：账户操作 ====================

void MainWindow::onLogin()
{
    Dialog dlg(m_db, Dialog::Login, this);
    if (dlg.exec() == QDialog::Accepted && dlg.isSuccessful()) {
        updateStatusBar();
        updateUIForRole();
        if (m_auth.isAdministrator()) {
            onRefreshUsers();
        }
    }
}

void MainWindow::onRegister()
{
    Dialog dlg(m_db, Dialog::Register, this);
    if (dlg.exec() == QDialog::Accepted && dlg.isSuccessful()) {
        updateStatusBar();
        updateUIForRole();
    }
}

void MainWindow::onGuestMode()
{
    m_auth.loginAsGuest();
    updateStatusBar();
    updateUIForRole();
    ui->statusbar->showMessage(QStringLiteral("游客模式 - 仅限查询"), 3000);
}

void MainWindow::onLogout()
{
    m_auth.logout();
    updateStatusBar();
    updateUIForRole();
    m_trainTable->setRowCount(0);
    m_ticketTable->setRowCount(0);
}

void MainWindow::onExit()
{
    close();
}

// ==================== 槽函数：列车查询 ====================

void MainWindow::onSearchTrains()
{
    QString trainNo = m_trainNoEdit->text().trimmed();
    QString from = m_fromStationEdit->text().trimmed();
    QDate date = m_dateEdit->date();

    QVector<Train> results = m_trains.searchTrains(trainNo, from, date);

    m_trainTable->setRowCount(results.size());
    for (int i = 0; i < results.size(); ++i) {
        const Train &t = results[i];
        m_trainTable->setItem(i, 0, new QTableWidgetItem(t.trainNo));
        m_trainTable->setItem(i, 1, new QTableWidgetItem(t.fromStation));
        m_trainTable->setItem(i, 2, new QTableWidgetItem(t.toStation));
        m_trainTable->setItem(i, 3, new QTableWidgetItem(t.departTime));
        m_trainTable->setItem(i, 4, new QTableWidgetItem(t.arriveTime));
        m_trainTable->setItem(i, 5, new QTableWidgetItem(QStringLiteral("¥%1").arg(t.basePrice, 0, 'f', 2)));
        m_trainTable->setItem(i, 6, new QTableWidgetItem(t.status));
    }

    ui->statusbar->showMessage(
        QStringLiteral("车次搜索结果: 共 %1 条").arg(results.size()), 5000);
}

void MainWindow::onQueryTickets()
{
    QString from = m_ticketFromEdit->text().trimmed();
    QString to = m_ticketToEdit->text().trimmed();
    QDate date = m_ticketDateEdit->date();

    if (from.isEmpty() || to.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入出发站和到达站"));
        return;
    }

    QVector<TrainTicketInfo> results = m_trains.queryRemainingTickets(from, to, date);

    m_ticketTable->setRowCount(results.size());
    for (int i = 0; i < results.size(); ++i) {
        const TrainTicketInfo &t = results[i];
        m_ticketTable->setItem(i, 0, new QTableWidgetItem(t.trainNo));
        m_ticketTable->setItem(i, 1, new QTableWidgetItem(t.fromStation));
        m_ticketTable->setItem(i, 2, new QTableWidgetItem(t.toStation));
        m_ticketTable->setItem(i, 3, new QTableWidgetItem(t.travelDate.toString("yyyy-MM-dd")));
        m_ticketTable->setItem(i, 4, new QTableWidgetItem(t.departTime));
        m_ticketTable->setItem(i, 5, new QTableWidgetItem(t.arriveTime));
        m_ticketTable->setItem(i, 6, new QTableWidgetItem(t.seatType));
        m_ticketTable->setItem(i, 7, new QTableWidgetItem(QStringLiteral("¥%1").arg(t.price, 0, 'f', 2)));
        m_ticketTable->setItem(i, 8, new QTableWidgetItem(QStringLiteral("%1 张").arg(t.remainingSeats)));
    }

    ui->statusbar->showMessage(
        QStringLiteral("余票查询结果: 共 %1 条").arg(results.size()), 5000);
}

// ==================== 槽函数：用户管理 ====================

void MainWindow::onRefreshUsers()
{
    if (!m_auth.isAdministrator()) return;

    QVector<User> consumers = m_users.listConsumers();

    m_userTable->setRowCount(consumers.size());
    for (int i = 0; i < consumers.size(); ++i) {
        const User &u = consumers[i];
        m_userTable->setItem(i, 0, new QTableWidgetItem(QString::number(u.id)));
        m_userTable->setItem(i, 1, new QTableWidgetItem(u.username));
        m_userTable->setItem(i, 2, new QTableWidgetItem(u.role));
        m_userTable->setItem(i, 3, new QTableWidgetItem(u.enabled ? QStringLiteral("启用") : QStringLiteral("禁用")));
        m_userTable->setItem(i, 4, new QTableWidgetItem(u.createdAt));
    }
}

void MainWindow::onToggleUserEnabled()
{
    if (!m_auth.isAdministrator()) return;

    int row = m_userTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择要操作的用户"));
        return;
    }

    QTableWidgetItem *idItem = m_userTable->item(row, 0);
    if (!idItem) return;

    int userId = idItem->text().toInt();
    QTableWidgetItem *statusItem = m_userTable->item(row, 3);
    bool currentlyEnabled = (statusItem && statusItem->text() == QStringLiteral("启用"));

    QString err;
    if (m_users.setEnabled(userId, !currentlyEnabled, &err)) {
        onRefreshUsers();
        ui->statusbar->showMessage(
            currentlyEnabled ? QStringLiteral("用户已禁用") : QStringLiteral("用户已启用"), 3000);
    } else {
        QMessageBox::critical(this, QStringLiteral("操作失败"), err);
    }
}
