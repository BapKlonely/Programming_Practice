#include "UserService.h"
#include "database.h"
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

UserService::UserService(Database &db)
    : m_db(db)
{
}

UserService::~UserService()=default;

bool UserService::setEnabled(int userId,bool enabled,QString* err)
{
    QSqlDatabase sqlDb=m_db.connection();
    QSqlQuery q(sqlDb);
    q.prepare(QStringLiteral("UPDATE users SET enabled=? WHERE id=?"));
    q.addBindValue(enabled ? 1 : 0);
    q.addBindValue(userId);
    if(!q.exec())
    {
        if(err) *err=q.lastError().text();
        return false;
    }
    return true;
}

QVector<User> UserService::listConsumers()
{
    QVector<User> result;
    QSqlDatabase sqlDb=m_db.connection();
    QSqlQuery q(sqlDb);
    q.prepare(QStringLiteral(
        "SELECT id,username,password_hash,salt,role,enabled,created_at "
        "FROM users WHERE role='consumer'"));
    q.exec();
    while(q.next())
        result.append(rowToUser(q));
    return result;
}