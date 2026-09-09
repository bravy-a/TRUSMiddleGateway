#include "trusdbworker.h"

#include <QSqlQuery>
#include <limits>
#include <span>
#include <stdexcept>

TRUSDBWorker::TRUSDBWorker(DBPayload payload, QString connectionName, QObject* parent)
    : QObject(parent), m_dbPayload(std::move(payload)), m_connectionName(std::move(connectionName))
{}

namespace {
    // Move this later
    constexpr QStringView PriceDataInsertSql = uR"(
        INSERT INTO [ED].[dbo].[PRICE_TRUS] (
            [UpdateTime], [StockCode], [StockName], [Status], [PreviousPrice], [OpenPrice], [HighestPrice], [LowestPrice],
            [LastPrice], [LastVolume], [Change], [ChangePercentage], [Bid], [BidVolume], [Offer], [OfferVolume],
            [TotalFrequency], [TotalVolume], [TotalValue], [TotalAllFreq], [TotalAllVolume], [TotalAllValue],
            [SpNotation], [IEPriceOp], [IEVolOp], [IEPriceCl], [IEVolCl], [IEPriceSpMonitoring], [IEVolSpMonitoring],
            [BBidIEP], [BBidIEV], [BOfferIEP], [BOfferIEV]
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";

    qint32 toSqlInt(std::uint32_t value) {
        if (value > static_cast<std::uint32_t>(std::numeric_limits<qint32>::max()))
            throw std::overflow_error("Value exceeds SQL Server INT range.");
        return static_cast<qint32>(value);
    }

    qlonglong toSqlBigInt(std::uint64_t value) {
        if (value > static_cast<std::uint64_t>(std::numeric_limits<qlonglong>::max()))
            throw std::overflow_error("Value exceeds SQL Server BIGINT range.");
        return static_cast<qlonglong>(value);
    }
}

void TRUSDBWorker::initialize() {
    if (m_db) return;

    try {
        m_db = std::make_unique<MSDB>(m_dbPayload, m_connectionName);
        emit initialized();
    }
    catch (const std::exception& e) {
        emit errorOccurred(QString::fromUtf8(e.what()));
    }
}

void TRUSDBWorker::uploadPriceData(std::vector<PriceDataRow> rows) {
    if (rows.empty()) return;

    if (!m_db) {
        emit errorOccurred(QStringLiteral("TRUS database is not initialized."));
        return;
    }

    try {
        const qsizetype rowCount = static_cast<qsizetype>(rows.size());

        m_db->executePrepared(PriceDataInsertSql, std::span<const PriceDataRow>{rows}, [](QSqlQuery& query, const PriceDataRow& row) {
            query.bindValue(0, row.updateTime);
            query.bindValue(1, row.stockCode);
            query.bindValue(2, row.stockName);
            query.bindValue(3, row.status);
            query.bindValue(4, toSqlInt(row.previousPrice));
            query.bindValue(5, toSqlInt(row.openPrice));
            query.bindValue(6, toSqlInt(row.highestPrice));
            query.bindValue(7, toSqlInt(row.lowestPrice));
            query.bindValue(8, toSqlInt(row.lastPrice));
            query.bindValue(9, toSqlInt(row.lastVolume));
            query.bindValue(10, static_cast<qint32>(row.change));
            query.bindValue(11, row.changePercentage);
            query.bindValue(12, toSqlInt(row.bid));
            query.bindValue(13, toSqlBigInt(row.bidVolume));
            query.bindValue(14, toSqlInt(row.offer));
            query.bindValue(15, toSqlBigInt(row.offerVolume));
            query.bindValue(16, toSqlBigInt(row.totalFrequency));
            query.bindValue(17, toSqlBigInt(row.totalVolume));
            query.bindValue(18, toSqlBigInt(row.totalValue));
            query.bindValue(19, toSqlBigInt(row.totalAllFreq));
            query.bindValue(20, toSqlBigInt(row.totalAllVolume));
            query.bindValue(21, toSqlBigInt(row.totalAllValue));
            query.bindValue(22, row.spNotation);
            query.bindValue(23, toSqlInt(row.IEPriceOp));
            query.bindValue(24, toSqlInt(row.IEVolOp));
            query.bindValue(25, toSqlInt(row.IEPriceCl));
            query.bindValue(26, toSqlInt(row.IEVolCl));
            query.bindValue(27, toSqlInt(row.IEPriceSpMonitoring));
            query.bindValue(28, toSqlInt(row.IEVolSpMonitoring));
            query.bindValue(29, toSqlInt(row.bBidIEP));
            query.bindValue(30, toSqlInt(row.bBidIEV));
            query.bindValue(31, toSqlInt(row.bOfferIEP));
            query.bindValue(32, toSqlInt(row.bOfferIEV));
        });

        emit uploadCompleted(rowCount);
    }
    catch (const std::exception& e) {
        emit errorOccurred(QString::fromUtf8(e.what()));
    }
}

void TRUSDBWorker::shutdown() {
    m_db.reset();
}