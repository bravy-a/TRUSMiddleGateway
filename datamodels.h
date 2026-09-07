#pragma once

#include <cstddef>
#include <cstdint>
#include <QString>

constexpr std::size_t maxStockCodeLength = 10;
constexpr std::size_t maxMarketCodeLength = 2;

enum class InstrumentType {
    ORDI,           // Ordinary Stock
    ORDI_PREOPEN,   // Ordinary Stock Preopening
    MUTI,           // Mutual Fund ETF
    WARI,           // Warrant
    RGHI,           // Right
    ACCEL,          // Acceleration Board
    S_WARI,         // Structured Warrant
    WATCHLIST,      // Special Monitoring
    WATCH_CALL,     // Call Auction
    WARI_CALL,      // Warrant Call Auction
    RGHI_CALL       // Right Call Auction
};

enum class MarketCode {
    RG,
    NG,
    TN
};

[[nodiscard]] constexpr std::string_view toString(MarketCode code) {
    switch (code) {
        case MarketCode::RG: return "RG";
        case MarketCode::NG: return "NG";
        case MarketCode::TN: return "TN";
    }

    return {};
}

[[nodiscard]] MarketCode toMarketCode(const QString& code)
{
    if (code == "RG") return MarketCode::RG;
    if (code == "NG") return MarketCode::NG;
    if (code == "TN") return MarketCode::TN;

    throw std::invalid_argument("Invalid market code");
}

struct OrderLeg {
    std::uint64_t volume;
    std::uint32_t price;
};

struct TradeLeg {
    std::uint64_t volume;
    std::uint64_t frequency;
    std::uint32_t price;
};

struct StockData
{
    StockData(); // Just to reserve memory for QString

    QString stockCode;

    double changePercentage;

    std::uint64_t bidVolume {};
    std::uint64_t offerVolume {};
    std::uint64_t totalFrequency {};
    std::uint64_t totalVolume {};
    std::uint64_t totalValue {};
    std::uint32_t previousPrice {};
    std::uint32_t openPrice {};
    std::uint32_t highestPrice {};
    std::uint32_t lowestPrice {};
    std::uint32_t lastPrice {};
    std::uint32_t lastVolume {};
    std::int32_t change {};
    std::uint32_t bid {};
    std::uint32_t offer {};

    [[nodiscard]] QString toString() const;
};

struct StockOrderBook {
    StockOrderBook(); // Memory reserve

    QString stockCode;
    MarketCode marketCode;
    std::uint32_t bidLegAmount;
    std::vector<OrderLeg> bidLegs;
    std::uint32_t askLegAmount;
    std::vector<OrderLeg> askLegs;
};

struct StockTradeBook {
    StockTradeBook(); // Memory Reserve

    QString stockCode;
    MarketCode marketCode;
    std::uint32_t tradeLegAmount;
    std::vector<TradeLeg> tradeLegs;
};

struct InitialStockInfo
{
    InitialStockInfo();

    QString stockCode;

    StockOrderBook stockOrderBook;
    StockTradeBook stockTradeBook;

    double priceDecimal;
    double sharesPerLot;

    std::uint64_t totalFrequency;
    std::uint64_t totalVolume;
    std::uint64_t totalValue;
    std::uint64_t IEV;
    std::uint64_t IEVClosing;

    std::uint32_t previousPrice;
    std::uint32_t openPrice;
    std::uint32_t highPrice;
    std::uint32_t lowPrice;
    std::uint32_t lastPrice;
    std::uint32_t IEP;
    std::uint32_t IEPClosing;

    MarketCode marketCode;
    InstrumentType instrumentType;
};

using ParsedDataTypes = std::variant<StockOrderBook, StockTradeBook, InitialStockInfo>;