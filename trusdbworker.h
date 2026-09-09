#pragma once

#include "sql.h"
#include "datamodels.h"

#include <QObject>
#include <QString>
#include <memory>
#include <vector>

class TRUSDBWorker final : public QObject
{
    Q_OBJECT

public:
    explicit TRUSDBWorker(DBPayload payload, QString connectionName = QStringLiteral("TRUS_DB"), QObject* parent = nullptr);
    void uploadPriceData(std::vector<PriceDataRow> rows);

public slots:
    void initialize();
    void shutdown();

signals:
    void initialized();
    void uploadCompleted(qsizetype rowCount);
    void errorOccurred(const QString& error);

private:
    DBPayload m_dbPayload;
    QString m_connectionName;
    std::unique_ptr<MSDB> m_db;
};