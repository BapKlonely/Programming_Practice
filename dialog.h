#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

#include "core/database.h"
#include "services/AuthService.h"

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode { Login, Register };

    explicit Dialog(Database &db, Mode mode = Login, QWidget *parent = nullptr);
    ~Dialog();

    /// 登录成功时返回登录的用户对象
    User getLoggedInUser() const { return m_user; }
    /// 是否成功完成登录/注册
    bool isSuccessful() const { return m_successful; }

private slots:
    void onSubmit();

private:
    void setupUI();
    void setMode(Mode mode);

    Ui::Dialog *ui;
    Database &m_db;
    AuthService m_auth;
    Mode m_mode;
    User m_user;
    bool m_successful = false;

    QLabel *m_titleLabel;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_confirmPasswordEdit;
    QLabel *m_confirmLabel;
    QPushButton *m_submitBtn;
    QPushButton *m_cancelBtn;
    QLabel *m_messageLabel;
};
