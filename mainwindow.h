#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QDateEdit>
#include <QAction>

#include "core/database.h"
#include "services/AuthService.h"
#include "services/TrainService.h"
#include "services/UserService.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLogin();
    void onRegister();
    void onGuestMode();
    void onLogout();
    void onSearchTrains();
    void onQueryTickets();
    void onRefreshUsers();
    void onToggleUserEnabled();
    void onExit();

private:
    void setupUI();
    void setupMenuBar();
    QWidget* createTrainSearchPage();
    QWidget* createUserManagementPage();
    void updateStatusBar();
    void updateUIForRole();

    Ui::MainWindow *ui;
    Database &m_db;
    AuthService m_auth;
    TrainService m_trains;
    UserService m_users;

    // 页面切换
    QStackedWidget *m_stack;
    QTabWidget *m_tabs;

    // 菜单项（需要根据登录状态启用/禁用）
    QAction *m_loginAction;
    QAction *m_registerAction;
    QAction *m_guestAction;
    QAction *m_logoutAction;

    // 列车查询页控件
    QLineEdit *m_trainNoEdit;
    QLineEdit *m_fromStationEdit;
    QDateEdit *m_dateEdit;
    QPushButton *m_searchBtn;
    QTableWidget *m_trainTable;

    QLineEdit *m_ticketFromEdit;
    QLineEdit *m_ticketToEdit;
    QDateEdit *m_ticketDateEdit;
    QPushButton *m_queryTicketBtn;
    QTableWidget *m_ticketTable;

    // 用户管理页控件
    QTableWidget *m_userTable;
    QPushButton *m_refreshUsersBtn;
    QPushButton *m_toggleUserBtn;
};
