#include "datamodels.h"

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

// UpdateStockTradeBook Class
StockTradeBook::StockTradeBook () {
    stockCode.reserve(maxStockCodeLength);
}

// InitialStockInfo Class
InitialStockInfo::InitialStockInfo () {
    stockCode.reserve(maxStockCodeLength);
}

// Trade Class
Trade::Trade () {
    stockCode.reserve(maxStockCodeLength);
}

// Indicative Equilibrium Data
IndicativeEquilibriumData::IndicativeEquilibriumData () {
    stockCode.reserve(maxStockCodeLength);
}