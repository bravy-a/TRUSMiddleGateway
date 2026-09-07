#include "marketinfoparser.h"
#include "datamodels.h"
#include <QByteArray>
#include <QByteArrayView>
#include <QString>

#include <charconv>
#include <cstdint>
#include <stdexcept>
#include <string_view>

MarketInfoParser::MarketInfoParser(QObject *parent)
    :QObject(parent)
{};

ParsedDataTypes MarketInfoParser::ParseMessage(const QByteArray& msg) {
    switch (msg.at(0)) {
    case 'v':
        return ParseStockOrderBook(msg);
    case 'x':
        return ParseStockTradeBook(msg);
    case 'y':
        return ParseInitialStockInfo(msg);
    default:
        std::terminate(); // Change this later
    }
}

const QByteArrayView NextField(QByteArrayView& view, qsizetype&cursor) {
    const qsizetype start {cursor};
    while ((cursor < view.size()) && (view[cursor] != '|')) ++cursor;

    const QByteArrayView field {view.sliced(start, cursor - start)};
    if (cursor < view.size()) ++cursor;

    return field;
};

StockOrderBook MarketInfoParser::ParseStockOrderBook(const QByteArray& msg)
{
    StockOrderBook stockOrderBook {};
    const QByteArrayView view {msg};
    qsizetype cursor {0};

    const QByteArrayView messageType {NextField(view, cursor)};
    if ((messageType.size() != 1) || (messageType[0] != 'v'))
        throw std::invalid_argument("Invalid StockOrderBook message type");

    stockOrderBook.stockCode = QString::fromLatin1(NextField(view, cursor));
    stockOrderBook.marketCode = toMarketCode(QString::fromLatin1(NextField(view, cursor)));
    stockOrderBook.bidLegAmount = ParseInteger<std::uint32_t>(NextField(view, cursor));
    stockOrderBook.bidLegs.reserve(stockOrderBook.bidLegAmount);

    for (std::uint32_t i {0}; i < stockOrderBook.bidLegAmount; ++i) {
        const auto price {ParseInteger<std::uint32_t>(NextField(view, cursor))};
        const auto volume {ParseInteger<std::uint64_t>(NextField(view, cursor))};
        stockOrderBook.bidLegs.emplace_back(OrderLeg {volume, price});
    }

    stockOrderBook.askLegAmount = ParseInteger<std::uint32_t>(NextField(view, cursor));
    stockOrderBook.askLegs.reserve(stockOrderBook.askLegAmount);

    for (std::uint32_t i {0}; i < stockOrderBook.askLegAmount; ++i) {
        const auto price {ParseInteger<std::uint32_t>(NextField(view, cursor))};
        const auto volume {ParseInteger<std::uint64_t>(NextField(view, cursor))};
        stockOrderBook.askLegs.emplace_back(OrderLeg {volume, price});
    }

    if (cursor != view.size())
        throw std::invalid_argument("Unexpected fields after StockOrderBook");

    return stockOrderBook;
}

StockTradeBook MarketInfoParser::ParseStockTradeBook(const QByteArray& msg) {
    StockTradeBook stockTradeBook{};
    const QByteArrayView view {msg};
    qsizetype cursor {0};

    const QByteArrayView messageType{NextField(view, cursor)};
    if ((messageType.size() != 1) || (messageType[0] != 'v'))
        throw std::invalid_argument("Invalid message type");

    stockTradeBook.stockCode = QString::fromLatin1(NextField(view, cursor));
    stockTradeBook.marketCode = toMarketCode(QString::fromLatin1(NextField(view, cursor)));
    stockTradeBook.tradeLegAmount = ParseInteger<std::uint32_t>(NextField(view, cursor));
    for (std::uint32_t i {0}; i < stockTradeBook.tradeLegAmount; i++) {
        const auto price {ParseInteger<std::uint32_t>(NextField(view, cursor))};
        const auto volume {ParseInteger<std::uint64_t>(NextField(view, cursor))};
    }

    if (cursor != view.size()) throw std::invalid_argument("Unexpected fields after StockTradeBook");

    return stockTradeBook;
}

InitialStockInfo MarketInfoParser::ParseInitialStockInfo(const QByteArray& msg) {

}

