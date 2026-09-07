#include "datamodels.h"

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