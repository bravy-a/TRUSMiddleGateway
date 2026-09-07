#pragma once

#include "datamodels.h"
#include <QObject>
#include <charconv>


class MarketInfoParser : public QObject
{
    Q_OBJECT
public:
    MarketInfoParser(std::chrono::year_month_day date, QObject *parent = nullptr);

    ParsedDataTypes ParseMessage(const QByteArray& msg);

private:
    template <GenericInteger I>
    static I ParseInteger(QByteArrayView field)
    {
        I value {};
        const auto [ptr, ec] = std::from_chars(field.begin(), field.end(), value);

        if ((field.isEmpty()) || (ec != std::errc {}) || (ptr != field.end()))
            throw std::invalid_argument("Invalid numeric field");

        return value;
    }

    // Helper lambda to iterate over "|" symbols
    const QByteArrayView NextField(QByteArrayView view, qsizetype& cursor);
    [[nodiscard]] sys_datetime ParseTimestamp (const std::chrono::year_month_day tradingDate, const QByteArrayView field);

    StockOrderBook ParseStockOrderBook(const QByteArray& msg);
    StockTradeBook ParseStockTradeBook(const QByteArray& msg);
    InitialStockInfo ParseInitialStockInfo(const QByteArray& msg);
    Trade ParseTrade(const std::chrono::year_month_day date, const QByteArray& msg);

    // Helper attributes
    std::chrono::year_month_day m_date; // Used to indicate the current trade date for the timestamp.

};