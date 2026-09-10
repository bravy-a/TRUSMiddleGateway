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

std::pair<OrderLeg, OrderLeg> StockOrderBook::GetBestBidAsk() const {
    OrderLeg bestBid {};
    if (!bidLegs.empty()) bestBid = bidLegs[0];

    OrderLeg bestAsk {};
    if (!askLegs.empty()) bestAsk = askLegs[0];

    return std::pair(bestBid, bestAsk);
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

// PriceData Class
PriceData::PriceData(QObject *parent)
    : QObject(parent)
{}

void PriceData::UpdateInitialStockInfo(const InitialStockInfo& initialStockInfo) {
    if (initialStockInfo.marketCode != MarketCode::RG) return;

    const QString& stockCode = initialStockInfo.stockCode;
    if (!m_priceDataMapRG.contains(stockCode)) m_priceDataMapRG[stockCode] = {.stockCode=stockCode};

    PriceDataRow& row = m_priceDataMapRG[stockCode];

    row.stockName = {};
    row.status = {};
    row.spNotation = {};
    row.updateTime = QDateTime::currentDateTime();

    const auto [bestBid, bestAsk] = initialStockInfo.stockOrderBook.GetBestBidAsk();

    row.bidVolume = bestBid.volume;
    row.offerVolume = bestAsk.volume;
    row.totalFrequency = initialStockInfo.totalFrequency;
    row.totalVolume = initialStockInfo.totalVolume;
    row.totalValue = initialStockInfo.totalValue;
    row.totalAllFreq = 0;
    row.totalAllVolume = 0;
    row.totalAllValue = 0;

    row.previousPrice = initialStockInfo.previousPrice;
    row.openPrice = initialStockInfo.openPrice;
    row.highestPrice = initialStockInfo.highPrice;
    row.lowestPrice = initialStockInfo.lowPrice;
    row.lastPrice = initialStockInfo.lastPrice;
    row.lastVolume = 0; // TO DO: Verify if this is correct
    row.change = static_cast<std::int32_t>(initialStockInfo.lastPrice) - static_cast<std::int32_t>(initialStockInfo.previousPrice);
    row.bid = bestBid.price;
    row.offer = bestAsk.price;

    row.changePercentage = initialStockInfo.previousPrice == 0? 0.0 : static_cast<double>(row.change)/initialStockInfo.previousPrice;

    row.IEPriceOp = initialStockInfo.IEP;
    row.IEVolOp = initialStockInfo.IEV;
    row.IEPriceCl = initialStockInfo.IEPClosing;
    row.IEVolCl = initialStockInfo.IEVClosing;
    row.IEPriceSpMonitoring = 0;
    row.IEVolSpMonitoring = 0;
    row.bBidIEP = 0;
    row.bBidIEV = 0;
    row.bOfferIEP = 0;
    row.bOfferIEV = 0;
}

void PriceData::UpdateStockOrderBook(const StockOrderBook& stockOrderBook) {
    if (stockOrderBook.marketCode != MarketCode::RG) return;

    const QString& stockCode = stockOrderBook.stockCode;
    if (!m_priceDataMapRG.contains(stockCode)) m_priceDataMapRG[stockCode] = {.stockCode=stockCode};

    PriceDataRow& row = m_priceDataMapRG[stockCode];
    const auto [bestBid, bestAsk] = stockOrderBook.GetBestBidAsk();

    row.updateTime = QDateTime::currentDateTime();
    row.bid = bestBid.price;
    row.bidVolume = bestBid.volume;
    row.offer = bestAsk.price;
    row.offerVolume = bestAsk.volume;
}

void PriceData::UpdateStockTradeBook(const StockTradeBook& stockTradeBook) {
    if (stockTradeBook.marketCode != MarketCode::RG) return;

    const QString& stockCode = stockTradeBook.stockCode;
    if (!m_priceDataMapRG.contains(stockCode)) m_priceDataMapRG[stockCode] = {.stockCode=stockCode};

    PriceDataRow& row = m_priceDataMapRG[stockCode];

    row.updateTime = QDateTime::currentDateTime();
    row.totalFrequency = stockTradeBook.totalFrequency;
    row.totalVolume = stockTradeBook.totalVolume;
    row.totalValue = stockTradeBook.totalValue;
}

void PriceData::UpdateTrade(const Trade& tradeData) {
    if (tradeData.marketCode != MarketCode::RG) return;

    const QString& stockCode = tradeData.stockCode;
    if (!m_priceDataMapRG.contains(stockCode)) m_priceDataMapRG[stockCode] = {.stockCode=stockCode};

    PriceDataRow& row = m_priceDataMapRG[stockCode];

    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(tradeData.tradeTime.time_since_epoch()).count();
    row.updateTime = QDateTime::fromMSecsSinceEpoch(milliseconds, QTimeZone("Asia/Jakarta"));
    row.lastPrice = tradeData.price;
    row.lastVolume = tradeData.volume;

    if (row.openPrice == 0) row.openPrice = tradeData.price;
    if (row.highestPrice == 0 || tradeData.price > row.highestPrice) row.highestPrice = tradeData.price;
    if (row.lowestPrice == 0 || tradeData.price < row.lowestPrice) row.lowestPrice = tradeData.price;

    ++row.totalFrequency;
    row.totalVolume += tradeData.volume;
    row.totalValue += static_cast<std::uint64_t>(tradeData.price) * tradeData.volume;

    row.change = static_cast<std::int32_t>(row.lastPrice) - static_cast<std::int32_t>(row.previousPrice);
    row.changePercentage = row.previousPrice == 0 ? 0.0 : static_cast<double>(row.change) / static_cast<double>(row.previousPrice);
}

void PriceData::UpdateIndicativeEquilibriumOpeningData(const IndicativeEquilibriumData& IEData) {
    if (IEData.marketCode != MarketCode::RG) return;

    const QString& stockCode = IEData.stockCode;
    if (!m_priceDataMapRG.contains(stockCode)) m_priceDataMapRG[stockCode] = {.stockCode=stockCode};

    PriceDataRow& row = m_priceDataMapRG[stockCode];

    row.updateTime = QDateTime::currentDateTime();
    row.IEPriceOp = IEData.IEP;
    row.IEVolOp = IEData.IEV;
}

void PriceData::UpdateIndicativeEquilibriumClosingData(const IndicativeEquilibriumData& IEData) {
    if (IEData.marketCode != MarketCode::RG) return;

    const QString& stockCode = IEData.stockCode;
    if (!m_priceDataMapRG.contains(stockCode)) m_priceDataMapRG[stockCode] = {.stockCode=stockCode};

    PriceDataRow& row = m_priceDataMapRG[stockCode];

    row.updateTime = QDateTime::currentDateTime();
    row.IEPriceCl = IEData.IEP;
    row.IEVolCl = IEData.IEV;
}

std::unordered_map<QString, PriceDataRow> PriceData::GetPriceDataSnapshot() const {
    return m_priceDataMapRG;
}