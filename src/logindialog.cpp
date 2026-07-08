#include "logindialog.h"

#include "customtitlebar.h"

#include <QBoxLayout>
#include <QFrame>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

LoginDialog::LoginDialog(DataStore &store, QWidget *parent)
    : QDialog(parent), m_store(store)
{
    setWindowTitle("火车票务系统 - 登录");
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setModal(true);
    resize(860, 460);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(new CustomTitleBar("火车票务系统 - 登录", this, false, this));

    auto *mainLayout = new QHBoxLayout();
    mainLayout->setContentsMargins(22, 22, 22, 22);
    mainLayout->setSpacing(18);

    auto *heroPanel = new QFrame(this);
    heroPanel->setObjectName("heroPanel");
    heroPanel->setMinimumWidth(300);
    auto *heroLayout = new QVBoxLayout(heroPanel);
    heroLayout->setContentsMargins(26, 28, 26, 28);

    auto *title = new QLabel("火车票务系统", heroPanel);
    title->setObjectName("heroTitle");
    title->setWordWrap(true);

    auto *subtitle = new QLabel("查询车次、在线购票、订单退票和车次管理的一体化桌面系统", heroPanel);
    subtitle->setObjectName("heroSubtitle");
    subtitle->setWordWrap(true);

    auto *badge = new QLabel("SQLite 数据库存储", heroPanel);
    badge->setObjectName("badge");

    auto *tips = new QLabel("管理员账号\nadmin / admin123", heroPanel);
    tips->setObjectName("heroSubtitle");
    tips->setWordWrap(true);

    heroLayout->addWidget(title);
    heroLayout->addSpacing(12);
    heroLayout->addWidget(subtitle);
    heroLayout->addSpacing(18);
    heroLayout->addWidget(badge, 0, Qt::AlignLeft);
    heroLayout->addStretch();
    heroLayout->addWidget(tips);

    auto *formPanel = new QFrame(this);
    formPanel->setObjectName("panel");
    auto *formPanelLayout = new QVBoxLayout(formPanel);
    formPanelLayout->setContentsMargins(22, 18, 22, 20);
    formPanelLayout->setSpacing(14);

    auto *formTitle = new QLabel("账号登录", formPanel);
    formTitle->setObjectName("sectionTitle");
    auto *formSubtitle = new QLabel("登录后进入主菜单，按功能按钮打开对应窗口", formPanel);
    formSubtitle->setObjectName("mutedLabel");

    auto *loginBox = new QGroupBox("用户登录", formPanel);
    auto *loginForm = new QFormLayout(loginBox);
    loginForm->setLabelAlignment(Qt::AlignRight);
    loginForm->setHorizontalSpacing(12);
    loginForm->setVerticalSpacing(10);
    m_loginUser = new QLineEdit(loginBox);
    m_loginUser->setPlaceholderText("请输入用户名");
    m_loginPass = new QLineEdit(loginBox);
    m_loginPass->setPlaceholderText("请输入密码");
    m_loginPass->setEchoMode(QLineEdit::Password);
    auto *loginButton = new QPushButton("登录系统", loginBox);
    connect(loginButton, &QPushButton::clicked, this, [this]() { login(); });
    loginForm->addRow("用户名", m_loginUser);
    loginForm->addRow("密码", m_loginPass);
    loginForm->addRow("", loginButton);

    auto *registerBox = new QGroupBox("新用户注册", formPanel);
    auto *registerForm = new QFormLayout(registerBox);
    registerForm->setLabelAlignment(Qt::AlignRight);
    registerForm->setHorizontalSpacing(12);
    registerForm->setVerticalSpacing(10);
    m_registerUser = new QLineEdit(registerBox);
    m_registerUser->setPlaceholderText("设置用户名");
    m_registerPass = new QLineEdit(registerBox);
    m_registerPass->setPlaceholderText("设置密码");
    m_registerPass->setEchoMode(QLineEdit::Password);
    m_registerName = new QLineEdit(registerBox);
    m_registerName->setPlaceholderText("乘车人姓名");
    m_registerPhone = new QLineEdit(registerBox);
    m_registerPhone->setPlaceholderText("手机号码");
    auto *registerButton = new QPushButton("创建账号", registerBox);
    registerButton->setObjectName("secondaryButton");
    connect(registerButton, &QPushButton::clicked, this, [this]() { registerAccount(); });
    registerForm->addRow("用户名", m_registerUser);
    registerForm->addRow("密码", m_registerPass);
    registerForm->addRow("姓名", m_registerName);
    registerForm->addRow("手机", m_registerPhone);
    registerForm->addRow("", registerButton);

    formPanelLayout->addWidget(formTitle);
    formPanelLayout->addWidget(formSubtitle);
    formPanelLayout->addWidget(loginBox);
    formPanelLayout->addWidget(registerBox);

    mainLayout->addWidget(heroPanel, 5);
    mainLayout->addWidget(formPanel, 7);
    rootLayout->addLayout(mainLayout);
}

QString LoginDialog::username() const
{
    return m_username;
}

bool LoginDialog::isAdmin() const
{
    return m_isAdmin;
}

void LoginDialog::login()
{
    const User *user = m_store.findUser(m_loginUser->text().trimmed());
    if (!user || user->password != m_loginPass->text()) {
        QMessageBox::warning(this, "登录失败", "用户名或密码错误");
        return;
    }
    m_username = user->username;
    m_isAdmin = user->admin;
    accept();
}

void LoginDialog::registerAccount()
{
    User user;
    user.username = m_registerUser->text().trimmed();
    user.password = m_registerPass->text();
    user.name = m_registerName->text().trimmed();
    user.phone = m_registerPhone->text().trimmed();

    QString error;
    if (!m_store.registerUser(user, &error)) {
        QMessageBox::warning(this, "注册失败", error);
        return;
    }

    QMessageBox::information(this, "注册成功", "账号已创建，请登录");
    m_registerUser->clear();
    m_registerPass->clear();
    m_registerName->clear();
    m_registerPhone->clear();
}
