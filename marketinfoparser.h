#pragma once

#include "datamodels.h"
#include <QObject>
#include <charconv>
#include <concepts>


template <typename T>
concept GenericInteger = std::integral<T> && !std::same_as<T, bool>; // All unsigned int types but exclude bool.

class MarketInfoParser : public QObject
{
    Q_OBJECT
public:
    MarketInfoParser(QObject *parent = nullptr);

    ParsedDataTypes ParseMessage(const QByteArray& msg);

private:
    static QByteArrayView NextField(QByteArrayView msg, qsizetype& cursor);

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
    const QByteArrayView NextField(QByteArrayView& view, qsizetype& cursor);

    StockOrderBook ParseStockOrderBook(const QByteArray& msg);
    StockTradeBook ParseStockTradeBook(const QByteArray& msg);
    InitialStockInfo ParseInitialStockInfo(const QByteArray& msg);

};