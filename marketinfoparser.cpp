#include "marketinfoparser.h"
#include <QByteArray>
#include <QByteArrayView>
#include <QString>
#include <cstdint>
#include <QDebug>
#include <stdexcept>
#include <chrono>


MarketInfoParser::MarketInfoParser(std::chrono::year_month_day date, QObject *parent)
    :m_date(date), QObject(parent)
{};

ParsedDataTypes MarketInfoParser::ParseMessage(const QByteArray& msg) {
    switch (msg.at(0)) {
    // Actual data
    case 'v':
        return ParseStockOrderBook(msg);
    case 'x':
        return ParseStockTradeBook(msg);
    case 'y':
        return ParseInitialStockInfo(msg);
    case 't':
        return ParseTrade(m_date, msg);
    case 'e':
        return ParseIndicativeEquilibriumOpeningData(msg);
    case 'f':
        return ParseIndicativeEquilibriumClosingData(msg);

    // Utilities (TO DO: Implement)
    case 'R':
        qInfo() << "LOGIN MESSAGE DETECTED!" + msg;
        return Trade{}; // TO DO
    case 'S':
        qInfo() << "HEARTBEAT DETECTED!" + msg;
        return Trade{}; // TO DO
    default:
        throw std::invalid_argument("cannot parse this message" + msg); // Change this later
    }
}

const QByteArrayView MarketInfoParser::NextField(QByteArrayView view, qsizetype&cursor) {
    const qsizetype start {cursor};
    while ((cursor < view.size()) && (view[cursor] != '|')) ++cursor;

    const QByteArrayView field {view.sliced(start, cursor - start)};
    if (cursor < view.size()) ++cursor;

    return field;
};

sys_datetime MarketInfoParser::ParseTimestamp(const std::chrono::year_month_day tradingDate, const QByteArrayView field)
{
    if (field.size() != 6) throw std::invalid_argument("Invalid HHMMSS field.");

    const auto hour = ParseInteger<std::uint16_t>(field.sliced(0, 2));
    const auto minute = ParseInteger<std::uint16_t>(field.sliced(2, 2));
    const auto second = ParseInteger<std::uint16_t>(field.sliced(4, 2));

    if (hour >= 24 || minute >= 60 || second >= 60) throw std::invalid_argument("Invalid HHMMSS time");

    const std::chrono::local_seconds localTime = std::chrono::local_days {tradingDate} + std::chrono::hours {hour} + std::chrono::minutes {minute} + std::chrono::seconds {second};
    return std::chrono::locate_zone("Asia/Jakarta")->to_sys(localTime);
}

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
    if ((messageType.size() != 1) || (messageType[0] != 'x'))
        throw std::invalid_argument("Invalid message type");

    stockTradeBook.stockCode = QString::fromLatin1(NextField(view, cursor));
    stockTradeBook.marketCode = toMarketCode(QString::fromLatin1(NextField(view, cursor)));
    stockTradeBook.tradeLegAmount = ParseInteger<std::uint32_t>(NextField(view, cursor));
    for (std::uint32_t i {0}; i < stockTradeBook.tradeLegAmount; i++) {
        const auto price {ParseInteger<std::uint32_t>(NextField(view, cursor))};
        const auto frequency {ParseInteger<std::uint32_t>(NextField(view, cursor))};
        const auto volume {ParseInteger<std::uint64_t>(NextField(view, cursor))};

        stockTradeBook.tradeLegs.emplace_back(TradeLeg{.volume=volume, .frequency=frequency, .price=price});
    }

    if (cursor != view.size()) throw std::invalid_argument("Unexpected fields after StockTradeBook");

    return stockTradeBook;
}

InitialStockInfo MarketInfoParser::ParseInitialStockInfo(const QByteArray& msg)
{
    InitialStockInfo initialStockInfo {};
    const QByteArrayView view {msg};
    qsizetype cursor {0};

    const QByteArrayView messageType {NextField(view, cursor)};
    if ((messageType.size() != 1) || (messageType[0] != 'y'))
        throw std::invalid_argument("Invalid InitialStockInfo message type");

    initialStockInfo.stockCode = QString::fromLatin1(NextField(view, cursor));
    initialStockInfo.marketCode = toMarketCode(QString::fromLatin1(NextField(view, cursor)));
    initialStockInfo.priceDecimal = ParseInteger<std::uint32_t>(NextField(view, cursor));
    initialStockInfo.sharesPerLot = ParseInteger<std::uint32_t>(NextField(view, cursor));
    initialStockInfo.instrumentType = toInstrumentType(QString::fromLatin1(NextField(view, cursor)));

    NextField(view, cursor); // This is the remark field. Currently we discard this.

    initialStockInfo.previousPrice = ParseInteger<std::uint32_t>(NextField(view, cursor));
    initialStockInfo.openPrice = ParseInteger<std::uint32_t>(NextField(view, cursor));
    initialStockInfo.highPrice = ParseInteger<std::uint32_t>(NextField(view, cursor));
    initialStockInfo.lowPrice = ParseInteger<std::uint32_t>(NextField(view, cursor));
    initialStockInfo.lastPrice = ParseInteger<std::uint32_t>(NextField(view, cursor));

    initialStockInfo.totalFrequency = ParseInteger<std::uint64_t>(NextField(view, cursor));
    initialStockInfo.totalVolume = ParseInteger<std::uint64_t>(NextField(view, cursor));
    initialStockInfo.totalValue = ParseInteger<std::uint64_t>(NextField(view, cursor));

    initialStockInfo.IEP = ParseInteger<std::uint32_t>(NextField(view, cursor));
    initialStockInfo.IEV = ParseInteger<std::uint64_t>(NextField(view, cursor));
    initialStockInfo.IEPClosing = ParseInteger<std::uint32_t>(NextField(view, cursor));
    initialStockInfo.IEVClosing = ParseInteger<std::uint64_t>(NextField(view, cursor));

    initialStockInfo.stockOrderBook.stockCode = initialStockInfo.stockCode;
    initialStockInfo.stockOrderBook.marketCode = initialStockInfo.marketCode;

    initialStockInfo.stockOrderBook.bidLegAmount = ParseInteger<std::uint32_t>(NextField(view, cursor));
    initialStockInfo.stockOrderBook.bidLegs.reserve(initialStockInfo.stockOrderBook.bidLegAmount);

    for (std::uint32_t i {0}; i < initialStockInfo.stockOrderBook.bidLegAmount; ++i) {
        const auto price {ParseInteger<std::uint32_t>(NextField(view, cursor))};
        const auto volume {ParseInteger<std::uint64_t>(NextField(view, cursor))};
        initialStockInfo.stockOrderBook.bidLegs.emplace_back(OrderLeg {volume, price});
    }

    initialStockInfo.stockOrderBook.askLegAmount = ParseInteger<std::uint32_t>(NextField(view, cursor));
    initialStockInfo.stockOrderBook.askLegs.reserve(initialStockInfo.stockOrderBook.askLegAmount);

    for (std::uint32_t i {0}; i < initialStockInfo.stockOrderBook.askLegAmount; ++i) {
        const auto price {ParseInteger<std::uint32_t>(NextField(view, cursor))};
        const auto volume {ParseInteger<std::uint64_t>(NextField(view, cursor))};
        initialStockInfo.stockOrderBook.askLegs.emplace_back(OrderLeg {volume, price});
    }

    initialStockInfo.stockTradeBook.stockCode = initialStockInfo.stockCode;
    initialStockInfo.stockTradeBook.marketCode = initialStockInfo.marketCode;
    initialStockInfo.stockTradeBook.tradeLegAmount = ParseInteger<std::uint32_t>(NextField(view, cursor));
    initialStockInfo.stockTradeBook.tradeLegs.reserve(initialStockInfo.stockTradeBook.tradeLegAmount);

    for (std::uint32_t i {0}; i < initialStockInfo.stockTradeBook.tradeLegAmount; ++i) {
        const auto price {ParseInteger<std::uint32_t>(NextField(view, cursor))};
        const auto frequency {ParseInteger<std::uint64_t>(NextField(view, cursor))};
        const auto volume {ParseInteger<std::uint64_t>(NextField(view, cursor))};
        initialStockInfo.stockTradeBook.tradeLegs.emplace_back(TradeLeg {volume, frequency, price});
    }

    if (cursor != view.size()) throw std::invalid_argument("Unexpected fields after InitialStockInfo");

    return initialStockInfo;
}

Trade MarketInfoParser::ParseTrade(const std::chrono::year_month_day date, const QByteArray& msg) {
    Trade trade {};
    const QByteArrayView view {msg};
    qsizetype cursor {0};

    const QByteArrayView messageType {NextField(view, cursor)};
    if ((messageType.size() != 1) || (messageType[0] != 't'))
        throw std::invalid_argument("Invalid Trade message type");

    trade.tradeTime = ParseTimestamp(date, (NextField(view, cursor)));
    trade.stockCode = QString::fromLatin1(NextField(view, cursor));
    trade.marketCode = toMarketCode(QString::fromLatin1(NextField(view, cursor)));
    trade.price = ParseInteger<std::uint32_t>(NextField(view, cursor));
    trade.volume = ParseInteger<std::uint32_t>(NextField(view, cursor));

    // The following 4 fields represent the buyer domicile, buyer code, seller domicile, and seller code. However these are hidden by IDX during trading hours so we skip them.
    NextField(view, cursor);
    NextField(view, cursor);
    NextField(view, cursor);
    NextField(view, cursor);

    if (cursor != view.size()) throw std::invalid_argument("Unexpected fields after Trade");

    return trade;
}

IndicativeEquilibriumData MarketInfoParser::ParseIndicativeEquilibriumOpeningData(const QByteArray& msg) {
    IndicativeEquilibriumData IEData {};
    const QByteArrayView view {msg};
    qsizetype cursor {0};

    const QByteArrayView messageType {NextField(view, cursor)};
    if ((messageType.size() != 1) || (messageType[0] != 'e'))
        throw std::invalid_argument("Invalid Trade message type");

    IEData.stockCode = QString::fromLatin1(NextField(view, cursor));
    IEData.marketCode = toMarketCode(QString::fromLatin1(NextField(view, cursor)));
    IEData.IEP = ParseInteger<std::uint32_t>(NextField(view, cursor));
    IEData.IEV = ParseInteger<std::uint64_t>(NextField(view, cursor));

    if (cursor != view.size()) throw std::invalid_argument("Unexpected fieds after indicative equilibrium data");

    return IEData;
}

IndicativeEquilibriumData MarketInfoParser::ParseIndicativeEquilibriumClosingData(const QByteArray& msg) {
    IndicativeEquilibriumData IEData {};
    const QByteArrayView view {msg};
    qsizetype cursor {0};

    const QByteArrayView messageType {NextField(view, cursor)};
    if ((messageType.size() != 1) || (messageType[0] != 'f'))
        throw std::invalid_argument("Invalid Trade message type");

    IEData.stockCode = QString::fromLatin1(NextField(view, cursor));
    IEData.marketCode = toMarketCode(QString::fromLatin1(NextField(view, cursor)));
    IEData.IEP = ParseInteger<std::uint32_t>(NextField(view, cursor));
    IEData.IEV = ParseInteger<std::uint64_t>(NextField(view, cursor));

    if (cursor != view.size()) throw std::invalid_argument("Unexpected fieds after indicative equilibrium data");

    return IEData;
}