#include "services/UserService.h"
#include "core/database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QCryptographicHash>
#include <QRandomGenerator>

//密码部分
static QString makeSalt()
{
    static const char pool[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const int n=static_cast<int>(sizeof(pool)-1);
    QString s;
    s.reserve(16);
    for (int i=0;i<16;++i)
        s.append(pool[QRandomGenerator::global()->bounded(n)]);
    return s;
}
static QString sha256(const QString &plain,const QString &salt)
{
    QByteArray input=(plain + salt).toUtf8();
    QByteArray hash=QCryptographicHash::hash(input,QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
}
static User rowToUser(const QSqlQuery &q)
{
    User u;
    u.id=q.value(0).toInt();
    u.username=q.value(1).toString();
    u.passwordHash=q.value(2).toString();
    u.salt=q.value(3).toString();
    u.role=q.value(4).toString();
    u.enabled=q.value(5).toBool();
    u.createdAt=q.value(6).toString();
    return u;
}

//构造和析构
UserService::UserService()
{
    QSqlQuery q(Database::instance().connection());
    q.exec(QStringLiteral("SELECT COUNT(*) FROM users"));
    if (q.next() && q.value(0).toInt()==0)
        createDefaultUsers();
}
UserService::~UserService()=default;

//判断用户类型
bool UserService::isAdmin(const User &user)
{
    return user.role=="admin";
}
bool UserService::isConsumer(const User &user)
{
    return user.role=="consumer";
}
bool UserService::isGuest(const User &user)
{
    return user.role.isEmpty()||user.role=="guest";
}

//创建默认用户
void UserService::createDefaultUsers()
{
    QSqlDatabase sqlDb=Database::instance().connection();
    QString now=QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    QSqlQuery q(sqlDb);
    //管理员admin/admin123
    QString s1=makeSalt();
    q.prepare(QStringLiteral(
        "INSERT INTO users (username,password_hash,salt,role,enabled,created_at) "
        "VALUES (?,?,?,'admin',1,?)"));
    q.addBindValue(QStringLiteral("admin"));
    q.addBindValue(sha256(QStringLiteral("admin123"),s1));
    q.addBindValue(s1);
    q.addBindValue(now);
    q.exec();
    //消费者user/user123
    QString s2=makeSalt();
    q.prepare(QStringLiteral(
        "INSERT INTO users (username,password_hash,salt,role,enabled,created_at) "
        "VALUES (?,?,?,'consumer',1,?)"));
    q.addBindValue(QStringLiteral("user"));
    q.addBindValue(sha256(QStringLiteral("user123"),s2));
    q.addBindValue(s2);
    q.addBindValue(now);
    q.exec();
}

//登录
void UserService::loginAsGuest()
{
    m_currentUser=User{};
    m_currentUser.role=QStringLiteral("guest");
    m_loggedIn=true;
    m_error.clear();
}

User* UserService::login(const QString& username,const QString& password,QString& error)
{
    QSqlDatabase sqlDb=Database::instance().connection();
    QSqlQuery q(sqlDb);
    q.prepare(QStringLiteral(
        "SELECT id,username,password_hash,salt,role,enabled,created_at "
        "FROM users WHERE username=?"));
    q.addBindValue(username);
    if (!q.exec() || !q.next())
    {
        error=QStringLiteral("用户不存在");
        return nullptr;
    }
    User u=rowToUser(q);
    if (!u.enabled)
    {
        error=QStringLiteral("账号已被禁用");
        return nullptr;
    }
    if (u.passwordHash!=sha256(password,u.salt))
    {
        error=QStringLiteral("密码错误");
        return nullptr;
    }
    m_currentUser=u;
    m_loggedIn=true;
    m_error.clear();
    return &m_currentUser;
}

//登出
void UserService::logout()
{
    m_currentUser=User{};
    m_loggedIn=false;
    m_error.clear();
}

//获取当前用户
const User* UserService::getCurrentUser() const
{
    return m_loggedIn? &m_currentUser:nullptr;
}

bool UserService::isLoggedIn() const
{
    return m_loggedIn;
}

bool UserService::isAdministrator() const
{
    return m_loggedIn && m_currentUser.role==QStringLiteral("admin");
}

//注册用户
bool UserService::registerUser(const User& user,QString& error)
{
    QSqlDatabase sqlDb=Database::instance().connection();
    //查重
    QSqlQuery ck(sqlDb);
    ck.prepare(QStringLiteral("SELECT COUNT(*) FROM users WHERE username=?"));
    ck.addBindValue(user.username);
    ck.exec();
    if (ck.next() && ck.value(0).toInt()>0)
    {
        error=QStringLiteral("用户名已存在");
        return false;
    }
    QString salt=makeSalt();
    QString hash=sha256(user.passwordHash,salt);
    QString now=QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    QSqlQuery q(sqlDb);
    q.prepare(QStringLiteral(
        "INSERT INTO users (username,password_hash,salt,role,enabled,created_at) "
        "VALUES (?,?,?,?,1,?)"));
    q.addBindValue(user.username);
    q.addBindValue(hash);
    q.addBindValue(salt);
    q.addBindValue(user.role);
    q.addBindValue(now);
    if (!q.exec())
    {
        error=Database::instance().lastError();
        return false;
    }
    m_error.clear();
    return true;
}
