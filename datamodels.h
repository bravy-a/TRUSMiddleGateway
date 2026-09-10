#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <QDateTime>
#include <QString>

// TO DO: Move this away later
constexpr std::size_t maxStockCodeLength = 10;
constexpr std::size_t maxMarketCodeLength = 2;
constexpr std::size_t tickerAmount = 1582; // Total number of tickers active in the exchange

template <typename T>
concept GenericInteger = std::integral<T> && !std::same_as<T, bool>; // All unsigned int types but exclude bool.

using sys_datetime = std::chrono::time_point<std::chrono::system_clock>;

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

// Helper
[[nodiscard]] constexpr std::string_view toString(MarketCode code);
[[nodiscard]] MarketCode toMarketCode(const QString& code);
[[nodiscard]] InstrumentType toInstrumentType(const QString& code);

struct OrderLeg {
    std::uint64_t volume;
    std::uint32_t price;

    [[nodiscard]] QString toString() const;
};

struct TradeLeg {
    std::uint64_t volume;
    std::uint64_t frequency;
    std::uint32_t price;

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

    std::pair<OrderLeg, OrderLeg> GetBestBidAsk() const;

    [[nodiscard]] QString toString() const;
};

struct StockTradeBook {
    StockTradeBook(); // Memory Reserve

    std::vector<TradeLeg> tradeLegs;

    QString stockCode;
    std::uint64_t totalFrequency {0};
    std::uint64_t totalVolume {0};
    std::uint64_t totalValue {0};

    MarketCode marketCode;
    std::uint32_t tradeLegAmount;

    [[nodiscard]] QString toString() const;
};

struct InitialStockInfo
{
    InitialStockInfo();

    QString stockCode;

    StockOrderBook stockOrderBook;
    StockTradeBook stockTradeBook;

    std::uint64_t totalFrequency;
    std::uint64_t totalVolume;
    std::uint64_t totalValue;
    std::uint64_t IEV;
    std::uint64_t IEVClosing;

    std::uint32_t priceDecimal;
    std::uint32_t sharesPerLot;
    std::uint32_t previousPrice;
    std::uint32_t openPrice;
    std::uint32_t highPrice;
    std::uint32_t lowPrice;
    std::uint32_t lastPrice;
    std::uint32_t IEP;
    std::uint32_t IEPClosing;

    MarketCode marketCode;
    InstrumentType instrumentType;

    [[nodiscard]] QString toString() const;
};

struct Trade {
    Trade();

    QString stockCode;
    std::uint32_t volume;
    std::uint32_t price;
    sys_datetime tradeTime;
    MarketCode marketCode;

    [[nodiscard]] QString toString() const;
};

struct IndicativeEquilibriumData {
    IndicativeEquilibriumData();

    QString stockCode;
    std::uint64_t IEV;
    std::uint32_t IEP;
    MarketCode marketCode;

    [[nodiscard]] QString toString() const;
};

struct PriceDataRow {
    QString stockCode;
    QString stockName;
    QString status;
    QString spNotation;
    QDateTime updateTime;

    double changePercentage;

    std::uint64_t bidVolume;
    std::uint64_t offerVolume;
    std::uint64_t totalFrequency;
    std::uint64_t totalVolume;
    std::uint64_t totalValue;
    std::uint64_t totalAllFreq;
    std::uint64_t totalAllVolume;
    std::uint64_t totalAllValue;

    std::uint32_t previousPrice;
    std::uint32_t openPrice;
    std::uint32_t highestPrice;
    std::uint32_t lowestPrice;
    std::uint32_t lastPrice;
    std::uint32_t lastVolume;
    std::int32_t change;
    std::uint32_t bid;
    std::uint32_t offer;

    std::uint32_t IEPriceOp;
    std::uint32_t IEVolOp;
    std::uint32_t IEPriceCl;
    std::uint32_t IEVolCl;
    std::uint32_t IEPriceSpMonitoring;
    std::uint32_t IEVolSpMonitoring;
    std::uint32_t bBidIEP;
    std::uint32_t bBidIEV;
    std::uint32_t bOfferIEP;
    std::uint32_t bOfferIEV;
};

class PriceData : public QObject
{
    Q_OBJECT
public:
    PriceData(QObject *parent = nullptr);

    void UpdateInitialStockInfo(const InitialStockInfo& initialStockInfo);
    void UpdateStockOrderBook(const StockOrderBook& stockOrderBook);
    void UpdateStockTradeBook(const StockTradeBook& stockTradeBook);
    void UpdateTrade(const Trade& tradeData);
    void UpdateIndicativeEquilibriumOpeningData(const IndicativeEquilibriumData& IEData);
    void UpdateIndicativeEquilibriumClosingData(const IndicativeEquilibriumData& IEData);

    [[nodiscard]] std::unordered_map<QString, PriceDataRow> GetPriceDataSnapshot() const;

private:
    std::unordered_map<QString, PriceDataRow> m_priceDataMapRG {}; // Regular Market Only
};

using ParsedDataTypes = std::variant<StockOrderBook, StockTradeBook, InitialStockInfo, Trade, IndicativeEquilibriumData>;