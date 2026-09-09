#pragma once
#include <QString>
#include <span>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringView>
#include <QSqlError>
#include <Quuid>

struct DBPayload {
    QString dbType {"QODBC"};
    QString driver;
    QString server;
    QString dbName;
    QString userName;
    QString userPassword;
    QString connectOptions;
};

class MSDB
{
public:
    explicit MSDB(DBPayload payload, QString connectionName = QStringLiteral("MSDB_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    ~MSDB();

    MSDB(const MSDB&) = delete;
    MSDB& operator=(const MSDB&) = delete;
    MSDB(MSDB&&) = delete;
    MSDB& operator=(MSDB&&) = delete;

    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] const QString& connectionName() const noexcept {return m_connectionName;};

    void exec(QStringView sql) const;

    template <typename T, typename Binder>
    void executePrepared(QStringView sql, std::span<const T> rows, Binder&& binder) const {
        if (rows.empty()) return;

        auto db = database();
        if (!db.transaction()) throwSqlError(u"transaction", db.lastError());

        try {
            {
                QSqlQuery query {db};
                if (!query.prepare(sql.toString())) throwSqlError(u"prepare", query.lastError());

                for (const T& row: rows) {
                    std::invoke(binder, query, row);
                    if (!query.exec()) throwSqlError(u"exec", query.lastError());
                }
            }

            if (!db.commit()) throwSqlError(u"commit", db.lastError());
        }
        catch(...) {
            db.rollback();
            throw;
        }
    }

private:
    [[nodiscard]] QSqlDatabase database() const;
    [[noreturn]] static void throwSqlError(QStringView operation, const QSqlError& error);
    QString m_connectionName;
};
