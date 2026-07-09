#include "AuthService.h"
#include "core/database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QCryptographicHash>
#include <QRandomGenerator>

static QString makeSalt()
{
    static const char pool[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const int n=static_cast<int>(sizeof(pool)-1);
    QString s;
    s.reserve(16);
    for(int i=0;i<16;++i)
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

AuthService::AuthService(Database &db)
    : m_db(db)
{
    QSqlQuery q(m_db.connection());
    q.exec(QStringLiteral("SELECT COUNT(*) FROM users"));
    if(q.next() && q.value(0).toInt()==0)
        createDefaultUsers();
}

AuthService::~AuthService()=default;

QString AuthService::getError() const
{
    return m_error;
}

// 判断用户类型
bool AuthService::isAdmin(const User &user)
{
    return user.role=="admin";
}

bool AuthService::isConsumer(const User &user)
{
    return user.role=="consumer";
}

bool AuthService::isGuest(const User &user)
{
    return user.role.isEmpty()||user.role=="guest";
}

// 默认用户
void AuthService::createDefaultUsers()
{
    QSqlDatabase sqlDb=m_db.connection();
    QString now=QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    QSqlQuery q(sqlDb);

    QString s1=makeSalt();
    q.prepare(QStringLiteral(
        "INSERT INTO users (username,password_hash,salt,role,enabled,created_at) "
        "VALUES (?,?,?,'admin',1,?)"));
    q.addBindValue(QStringLiteral("admin"));
    q.addBindValue(sha256(QStringLiteral("admin123"),s1));
    q.addBindValue(s1);
    q.addBindValue(now);
    q.exec();

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

// 会话
void AuthService::loginAsGuest()
{
    m_currentUser=User{};
    m_currentUser.role=QStringLiteral("guest");
    m_loggedIn=true;
    m_error.clear();
}

std::optional<User> AuthService::login(const QString& username,const QString& password)
{
    QSqlDatabase sqlDb=m_db.connection();
    QSqlQuery q(sqlDb);
    q.prepare(QStringLiteral(
        "SELECT id,username,password_hash,salt,role,enabled,created_at "
        "FROM users WHERE username=?"));
    q.addBindValue(username);
    if(!q.exec() || !q.next())
    {
        m_error=QStringLiteral("用户不存在");
        return std::nullopt;
    }
    User u=rowToUser(q);
    if(!u.enabled)
    {
        m_error=QStringLiteral("账号已被禁用");
        return std::nullopt;
    }
    if(u.passwordHash!=sha256(password,u.salt))
    {
        m_error=QStringLiteral("密码错误");
        return std::nullopt;
    }
    m_currentUser=u;
    m_loggedIn=true;
    m_error.clear();
    return u;
}

void AuthService::logout()
{
    m_currentUser=User{};
    m_loggedIn=false;
    m_error.clear();
}

const User* AuthService::getCurrentUser() const
{
    return m_loggedIn? &m_currentUser:nullptr;
}

bool AuthService::isLoggedIn() const
{
    return m_loggedIn;
}

bool AuthService::isAdministrator() const
{
    return m_loggedIn && m_currentUser.role==QStringLiteral("admin");
}

// 密码
bool AuthService::changePassword(int userId,const QString& oldPwd,const QString& newPwd,QString* err)
{
    QSqlDatabase sqlDb=m_db.connection();
    QSqlQuery q(sqlDb);

    q.prepare(QStringLiteral(
        "SELECT password_hash,salt FROM users WHERE id=?"));
    q.addBindValue(userId);
    if(!q.exec() || !q.next())
    {
        if(err) *err=QStringLiteral("用户不存在");
        return false;
    }

    QString oldHash=q.value(0).toString();
    QString oldSalt=q.value(1).toString();
    if(oldHash!=sha256(oldPwd,oldSalt))
    {
        if(err) *err=QStringLiteral("原密码错误");
        return false;
    }

    QString newSalt=makeSalt();
    QString newHash=sha256(newPwd,newSalt);

    q.prepare(QStringLiteral(
        "UPDATE users SET password_hash=?,salt=? WHERE id=?"));
    q.addBindValue(newHash);
    q.addBindValue(newSalt);
    q.addBindValue(userId);
    if(!q.exec())
    {
        if(err) *err=q.lastError().text();
        return false;
    }
    return true;
}

// consumer 管理
bool AuthService::createConsumer(const QString& username,const QString& password,QString* err)
{
    QSqlDatabase sqlDb=m_db.connection();

    QSqlQuery ck(sqlDb);
    ck.prepare(QStringLiteral("SELECT COUNT(*) FROM users WHERE username=?"));
    ck.addBindValue(username);
    ck.exec();
    if(ck.next() && ck.value(0).toInt()>0)
    {
        if(err) *err=QStringLiteral("用户名已存在");
        return false;
    }

    QString salt=makeSalt();
    QString hash=sha256(password,salt);
    QString now=QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    QSqlQuery q(sqlDb);
    q.prepare(QStringLiteral(
        "INSERT INTO users (username,password_hash,salt,role,enabled,created_at) "
        "VALUES (?,?,?,'consumer',1,?)"));
    q.addBindValue(username);
    q.addBindValue(hash);
    q.addBindValue(salt);
    q.addBindValue(now);
    if(!q.exec())
    {
        if(err) *err=q.lastError().text();
        return false;
    }
    return true;
}

// 查询
const User* AuthService::getUserByUsername(const QString& username) const
{
    QSqlDatabase sqlDb=m_db.connection();
    QSqlQuery q(sqlDb);
    q.prepare(QStringLiteral(
        "SELECT id,username,password_hash,salt,role,enabled,created_at "
        "FROM users WHERE username=?"));
    q.addBindValue(username);
    if(q.exec() && q.next())
    {
        static User u;
        u=rowToUser(q);
        return &u;
    }
    return nullptr;
}