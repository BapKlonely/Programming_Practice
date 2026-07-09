#include "dialog.h"
#include "ui_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>

Dialog::Dialog(Database &db, Mode mode, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
    , m_db(db)
    , m_auth(db)
    , m_mode(mode)
{
    ui->setupUi(this);
    setupUI();
    setMode(mode);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(30, 20, 30, 20);

    // 标题
    m_titleLabel = new QLabel;
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin-bottom: 10px;");
    mainLayout->addWidget(m_titleLabel);

    // 表单
    QFormLayout *formLayout = new QFormLayout;
    formLayout->setSpacing(8);

    m_usernameEdit = new QLineEdit;
    m_usernameEdit->setPlaceholderText(QStringLiteral("请输入用户名"));
    formLayout->addRow(QStringLiteral("用户名："), m_usernameEdit);

    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    formLayout->addRow(QStringLiteral("密码："), m_passwordEdit);

    // 注册模式才有确认密码
    m_confirmLabel = new QLabel(QStringLiteral("确认密码："));
    m_confirmPasswordEdit = new QLineEdit;
    m_confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    m_confirmPasswordEdit->setPlaceholderText(QStringLiteral("请再次输入密码"));
    formLayout->addRow(m_confirmLabel, m_confirmPasswordEdit);

    mainLayout->addLayout(formLayout);

    // 消息标签
    m_messageLabel = new QLabel;
    m_messageLabel->setStyleSheet("color: red;");
    m_messageLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_messageLabel);

    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();

    m_submitBtn = new QPushButton;
    m_submitBtn->setMinimumWidth(100);
    btnLayout->addWidget(m_submitBtn);

    m_cancelBtn = new QPushButton(QStringLiteral("取消"));
    m_cancelBtn->setMinimumWidth(100);
    btnLayout->addWidget(m_cancelBtn);

    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    mainLayout->addStretch();

    // 连接信号
    connect(m_submitBtn, &QPushButton::clicked, this, &Dialog::onSubmit);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void Dialog::setMode(Mode mode)
{
    m_mode = mode;
    if (mode == Login) {
        setWindowTitle(QStringLiteral("用户登录"));
        m_titleLabel->setText(QStringLiteral("用户登录"));
        m_submitBtn->setText(QStringLiteral("登录"));
        m_confirmLabel->hide();
        m_confirmPasswordEdit->hide();
    } else {
        setWindowTitle(QStringLiteral("用户注册"));
        m_titleLabel->setText(QStringLiteral("注册新用户"));
        m_submitBtn->setText(QStringLiteral("注册"));
        m_confirmLabel->show();
        m_confirmPasswordEdit->show();
    }
    m_messageLabel->clear();
}

void Dialog::onSubmit()
{
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.isEmpty()) {
        m_messageLabel->setText(QStringLiteral("请输入用户名"));
        return;
    }
    if (password.isEmpty()) {
        m_messageLabel->setText(QStringLiteral("请输入密码"));
        return;
    }

    if (m_mode == Login) {
        auto result = m_auth.login(username, password);
        if (result.has_value()) {
            m_user = result.value();
            m_successful = true;
            accept();
        } else {
            m_messageLabel->setText(QStringLiteral("登录失败：用户名或密码错误"));
        }
    } else {
        // 注册模式
        QString confirm = m_confirmPasswordEdit->text();
        if (confirm.isEmpty()) {
            m_messageLabel->setText(QStringLiteral("请确认密码"));
            return;
        }
        if (password != confirm) {
            m_messageLabel->setText(QStringLiteral("两次输入的密码不一致"));
            return;
        }

        QString err;
        if (m_auth.createConsumer(username, password, &err)) {
            // 注册成功，自动登录
            auto result = m_auth.login(username, password);
            if (result.has_value()) {
                m_user = result.value();
                m_successful = true;
            }
            accept();
        } else {
            m_messageLabel->setText(QStringLiteral("注册失败：%1").arg(err));
        }
    }
}
