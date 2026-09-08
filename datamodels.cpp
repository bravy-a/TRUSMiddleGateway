#include "datamodels.h"
#include <QDateTime>
#include <QStringList>
#include <QTimeZone>
#include <chrono>

// Helpers
constexpr std::string_view toString(MarketCode code) {
    switch (code) {
    case MarketCode::RG: return "RG";
    case MarketCode::NG: return "NG";
    case MarketCode::TN: return "TN";
    }

    return {};
}

MarketCode toMarketCode(const QString& code)
{
    if (code == "RG") return MarketCode::RG;
    if (code == "NG") return MarketCode::NG;
    if (code == "TN") return MarketCode::TN;

    throw std::invalid_argument("Invalid market code");
}

InstrumentType toInstrumentType(const QString& code) {
    if (code == "ORDI") return InstrumentType::ORDI;
    if (code == "ORDI_PREOPEN") return InstrumentType::ORDI_PREOPEN;
    if (code == "MUTI") return InstrumentType::MUTI;
    if (code == "WARI") return InstrumentType::WARI;
    if (code == "RGHI") return InstrumentType::RGHI;
    if (code == "ACCEL") return InstrumentType::ACCEL;
    if (code == "S_WARI") return InstrumentType::S_WARI;
    if (code == "WATCHLIST") return InstrumentType::WATCHLIST;
    if (code == "WATCH_CALL") return InstrumentType::WATCH_CALL;
    if (code == "WARI_CALL") return InstrumentType::WARI_CALL;
    if (code == "RGHI_CALL") return InstrumentType::RGHI_CALL;

    throw std::invalid_argument("Invalid instrument type");
};

// Leg Classes
QString OrderLeg::toString() const
{
    return QStringLiteral("OrderLeg{volume=%1, price=%2}").arg(volume).arg(price);
}

QString TradeLeg::toString() const
{
    return QStringLiteral("TradeLeg{volume=%1, frequency=%2, price=%3}").arg(volume).arg(frequency).arg(price);
}

// StockData Class
StockData::StockData() {
    stockCode.reserve(maxStockCodeLength);
}

QString StockData::toString() const
{
    return QStringLiteral(
               "StockData{"
               "stockCode=%1, "
               "previousPrice=%2, "
               "openPrice=%3, "
               "highestPrice=%4, "
               "lowestPrice=%5, "
               "lastPrice=%6, "
               "lastVolume=%7, "
               "change=%8, "
               "changePercentage=%9, "
               "bid=%10, "
               "bidVolume=%11, "
               "offer=%12, "
               "offerVolume=%13, "
               "totalFrequency=%14, "
               "totalVolume=%15, "
               "totalValue=%16"
               "}"
               )
        .arg(stockCode)
        .arg(previousPrice)
        .arg(openPrice)
        .arg(highestPrice)
        .arg(lowestPrice)
        .arg(lastPrice)
        .arg(lastVolume)
        .arg(change)
        .arg(changePercentage)
        .arg(bid)
        .arg(bidVolume)
        .arg(offer)
        .arg(offerVolume)
        .arg(totalFrequency)
        .arg(totalVolume)
        .arg(totalValue);
}

// UpdateStockOrderBook Class
StockOrderBook::StockOrderBook () {
    stockCode.reserve(maxStockCodeLength);
}

QString StockOrderBook::toString() const
{
    QStringList bids, asks;
    for (const auto& leg : bidLegs) bids.append(leg.toString());
    for (const auto& leg : askLegs) asks.append(leg.toString());
    return QStringLiteral("StockOrderBook{stockCode=%1, marketCode=%2, bidLegAmount=%3, bidLegs=[%4], askLegAmount=%5, askLegs=[%6]}")
        .arg(stockCode).arg(::toString(marketCode)).arg(bidLegAmount).arg(bids.join(", ")).arg(askLegAmount).arg(asks.join(", "));
}

// UpdateStockTradeBook Class
StockTradeBook::StockTradeBook () {
    stockCode.reserve(maxStockCodeLength);
}

QString StockTradeBook::toString() const
{
    QStringList trades;
    for (const auto& leg : tradeLegs) trades.append(leg.toString());
    return QStringLiteral("StockTradeBook{stockCode=%1, marketCode=%2, tradeLegAmount=%3, tradeLegs=[%4]}").arg(stockCode).arg(::toString(marketCode)).arg(tradeLegAmount).arg(trades.join(", "));
}

// InitialStockInfo Class
InitialStockInfo::InitialStockInfo () {
    stockCode.reserve(maxStockCodeLength);
}

QString InitialStockInfo::toString() const
{
    return QStringLiteral("InitialStockInfo{stockCode=%1, stockOrderBook=%2, stockTradeBook=%3, totalFrequency=%4, totalVolume=%5, totalValue=%6, IEV=%7, IEVClosing=%8, priceDecimal=%9, sharesPerLot=%10, previousPrice=%11, openPrice=%12, highPrice=%13, lowPrice=%14, lastPrice=%15, IEP=%16, IEPClosing=%17, marketCode=%18, instrumentType=%19}")
        .arg(stockCode).arg(stockOrderBook.toString()).arg(stockTradeBook.toString()).arg(totalFrequency).arg(totalVolume).arg(totalValue).arg(IEV).arg(IEVClosing).arg(priceDecimal).arg(sharesPerLot).arg(previousPrice).arg(openPrice).arg(highPrice).arg(lowPrice).arg(lastPrice).arg(IEP).arg(IEPClosing).arg(::toString(marketCode)).arg(static_cast<int>(instrumentType));
}

// Trade Class
Trade::Trade () {
    stockCode.reserve(maxStockCodeLength);
}

QString Trade::toString() const
{
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(tradeTime.time_since_epoch()).count();
    const QString time = QDateTime::fromMSecsSinceEpoch(milliseconds, QTimeZone("Asia/Jakarta")).toString("yyyy-MM-dd HH:mm:ss.zzz");
    return QStringLiteral("Trade{stockCode=%1, volume=%2, price=%3, tradeTime=%4, marketCode=%5}").arg(stockCode).arg(volume).arg(price).arg(time).arg(::toString(marketCode));
}

// Indicative Equilibrium Data
IndicativeEquilibriumData::IndicativeEquilibriumData () {
    stockCode.reserve(maxStockCodeLength);
}

QString IndicativeEquilibriumData::toString() const
{
    return QStringLiteral("IndicativeEquilibriumData{stockCode=%1, IEV=%2, IEP=%3, marketCode=%4}").arg(stockCode).arg(IEV).arg(IEP).arg(::toString(marketCode));
}