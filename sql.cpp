#include "sql.h"
#include <Quuid>

MSDB::MSDB(DBPayload payload, QString connectionName)
    : m_connectionName(std::move(connectionName))
{
    // Consider changing to reuse.
    if (QSqlDatabase::contains(m_connectionName)) throw std::runtime_error(QStringLiteral("SQL connection '%1' already exists.").arg(m_connectionName).toStdString());

    QSqlDatabase db = QSqlDatabase::addDatabase(payload.dbType, m_connectionName);
    db.setDatabaseName(QStringLiteral("Driver={%1};Server=%2;Database=%3;Uid=%4;Pwd=%5;")
        .arg(payload.driver, payload.server, payload.dbName, payload.userName, payload.userPassword));

    if (!payload.connectOptions.isEmpty()) db.setConnectOptions(payload.connectOptions);

    if (!db.open()) {
        const QSqlError error = db.lastError();
        db = {};
        QSqlDatabase::removeDatabase(m_connectionName);
        throwSqlError(u"database connection", error);
    }
}

MSDB::~MSDB() {
    if (!QSqlDatabase::contains(m_connectionName)) return;

    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isValid()) db.close();
    }

    QSqlDatabase::removeDatabase(m_connectionName);
}

bool MSDB::isOpen() const {
    if (!QSqlDatabase::contains(m_connectionName)) return false;
    const QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    return db.isValid() && db.isOpen();
}

QSqlDatabase MSDB::database() const {
    if (!QSqlDatabase::contains(m_connectionName)) throw std::runtime_error(QStringLiteral("SQL connection '%1' does not exist.").arg(m_connectionName).toStdString());

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);

    if (!db.isValid()) throw std::runtime_error(QStringLiteral("SQL connection '%1' is invalid.").arg(m_connectionName).toStdString());
    if (!db.isOpen()) throw std::runtime_error(QStringLiteral("SQL connection '%1' is not open.").arg(m_connectionName).toStdString());

    return db;
}

void MSDB::exec(QStringView sql) const {
    QSqlDatabase db = database();
    QSqlQuery query {db};
    if (!query.exec(sql.toString())) throwSqlError(u"exec", query.lastError());
}

void MSDB::throwSqlError(QStringView operation, const QSqlError& error) {
    throw std::runtime_error(QStringLiteral("%1 failed: %2").arg(operation.toString(), error.text()).toStdString());
}