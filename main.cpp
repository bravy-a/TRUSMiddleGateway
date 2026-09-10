#include <unordered_set>
#include <vector>
#include <type_traits>
#include <chrono>
#include <memory>
#include <utility>
#include <cstdint>
#include <QCoreApplication>
#include <QDateTime>
#include <QTimeZone>
#include <QThread>
#include <QTimer>
#include <QDebug>
#include "telnetreader.h"
#include "marketinfoparser.h"
#include "datamodels.h"
#include "trusdbworker.h"

using namespace std::chrono_literals;

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    // TRUS CREDS
    // TO DO move ts to a config file
    const QString hostName {};
    const quint16 port {};
    const QByteArray command {};

    // DB CREDS
    // TO DO
    DBPayload dbPayload;
    dbPayload.driver = "";
    dbPayload.server = "";
    dbPayload.dbName = "";
    dbPayload.userName = "";
    dbPayload.userPassword = "";

    if (dbPayload.driver.isEmpty() || dbPayload.server.isEmpty() || dbPayload.dbName.isEmpty() || dbPayload.userName.isEmpty() || dbPayload.userPassword.isEmpty()) {
        qCritical() << "TRUS database configuration is incomplete";
        return 1;
    }

    // TDate in WIB
    // TO DO
    const QDate qTradingDate {QDateTime::currentDateTimeUtc().toTimeZone(QTimeZone("Asia/Jakarta")).date()};
    const std::chrono::year_month_day tradingDate {
        std::chrono::year {qTradingDate.year()},
        std::chrono::month {static_cast<unsigned>(qTradingDate.month())},
        std::chrono::day {static_cast<unsigned>(qTradingDate.day())}
    };

    MarketInfoParser marketInfoParser {tradingDate};
    PriceData priceData;

    // Spawn DB worker on a separate thread
    QThread dbThread;
    auto* dbWorker = new TRUSDBWorker(std::move(dbPayload), QStringLiteral("TRUS_DB"));
    dbWorker->moveToThread(&dbThread);

    QObject::connect(&dbThread, &QThread::started, dbWorker, &TRUSDBWorker::initialize);
    QObject::connect(&dbThread, &QThread::finished, dbWorker, &QObject::deleteLater);

    // Establish telnet connection
    auto telnet = std::make_unique<TelnetReader>(hostName, port, [](TelnetReader& telnet) {
        qInfo() << "TRUS Logout Sequence";
        if (!telnet.isConnected()) return;
        telnet.writeLine("Q|");
        telnet.disconnectFromHost();
    });

    QObject::connect(telnet.get(), &TelnetReader::connected, telnet.get(), [&telnet, command] {
        telnet->writeLine(command);
        qInfo() << "Connected to TRUS";
    });

    QObject::connect(telnet.get(), &TelnetReader::disconnected, [] {qInfo() << "Disconnected from TRUS";});
    QObject::connect(telnet.get(), &TelnetReader::errorOcurred, [](const QString& error) {qWarning() << "Telnet error:" << error;});

    // Track changes to consider what has to be updated.
    std::unordered_set<QString> dirtyStocks, inFlightStocks;
    bool uploadInFlight {false};

    std::uint64_t parsedLines {0};

    const auto makeRows = [&priceData](const std::unordered_set<QString>& stocks) {
        auto snapshot = priceData.GetPriceDataSnapshot();
        std::vector<PriceDataRow> rows;
        rows.reserve(stocks.size());

        for (const QString& stockCode : stocks)
            if (auto it = snapshot.find(stockCode); it != snapshot.end()) rows.emplace_back(std::move(it->second));

        return rows;
    };

    const auto uploadRows = [dbWorker](std::vector<PriceDataRow> rows, Qt::ConnectionType connectionType) {
        QMetaObject::invokeMethod(dbWorker, [dbWorker, rows = std::move(rows)]() mutable {
            dbWorker->uploadPriceData(std::move(rows));
        }, connectionType);
    };

    // Main parser of each msg from TRUS
    // TO DO: After you clean up the parser you can clean up some of the branching here
    QObject::connect(telnet.get(), &TelnetReader::lineReceived, &app, [&](const QByteArray& line) {
        if (line.isEmpty()) return;

        const char messageType {line.front()};
        if (messageType == 'R' || messageType == 'S') return;

        try {
            const ParsedDataTypes parsedData {marketInfoParser.ParseMessage(line)};

            std::visit([&](const auto& data) {
                using T = std::decay_t<decltype(data)>;

                if (data.marketCode != MarketCode::RG) return;

                if constexpr (std::is_same_v<T, InitialStockInfo>) priceData.UpdateInitialStockInfo(data);
                else if constexpr (std::is_same_v<T, StockOrderBook>) priceData.UpdateStockOrderBook(data);
                else if constexpr (std::is_same_v<T, StockTradeBook>) priceData.UpdateStockTradeBook(data);
                else if constexpr (std::is_same_v<T, Trade>) priceData.UpdateTrade(data);
                else if constexpr (std::is_same_v<T, IndicativeEquilibriumData>) {
                    if (messageType == 'e') priceData.UpdateIndicativeEquilibriumOpeningData(data);
                    else if (messageType == 'f') priceData.UpdateIndicativeEquilibriumClosingData(data);
                }

                dirtyStocks.insert(data.stockCode);
            }, parsedData);

            ++parsedLines;
        }
        catch (const std::exception& e) {
            qWarning() << "Parse error:" << e.what() << "Message:" << line;
        }
    });

    // Batch upload to DB
    QTimer uploadTimer;
    uploadTimer.setInterval(1000);

    QObject::connect(&uploadTimer, &QTimer::timeout, &app, [&] {
        if (uploadInFlight || dirtyStocks.empty()) return;

        auto rows = makeRows(dirtyStocks);
        if (rows.empty()) {
            dirtyStocks.clear();
            return;
        }

        inFlightStocks.clear();
        inFlightStocks.swap(dirtyStocks);
        uploadInFlight = true;

        uploadRows(std::move(rows), Qt::QueuedConnection);
    });

    QObject::connect(dbWorker, &TRUSDBWorker::uploadCompleted, &app, [&](qsizetype rowCount) {
        uploadInFlight = false;
        inFlightStocks.clear();
        qInfo() << "Uploaded rows:" << rowCount;
    });

    QObject::connect(dbWorker, &TRUSDBWorker::errorOccurred, &app, [&](const QString& error) {
        if (uploadInFlight) {
            dirtyStocks.insert(inFlightStocks.begin(), inFlightStocks.end());
            inFlightStocks.clear();
            uploadInFlight = false;
        }

        qWarning() << "Database error:" << error;
    });

    // Benchmarking
    QTimer benchmarkTimer;
    benchmarkTimer.setInterval(1000);

    QObject::connect(&benchmarkTimer, &QTimer::timeout, [&] {
        qInfo() << "Parsed lines/sec:" << parsedLines;
        parsedLines = 0;
    });

    // TRUS data feed not started until the DB connection is established
    QObject::connect(dbWorker, &TRUSDBWorker::initialized, &app, [&] {
        qInfo() << "TRUS database initialized";
        uploadTimer.start();
        benchmarkTimer.start();
        telnet->connectToHost();
    });

    // Clean shutdown
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [&] {
        uploadTimer.stop();
        benchmarkTimer.stop();

        telnet.reset();

        // Flush anything that changed after the previous batch was started
        if (!dirtyStocks.empty()) {
            auto rows = makeRows(dirtyStocks);
            if (!rows.empty()) uploadRows(std::move(rows), Qt::BlockingQueuedConnection);
        }

        QMetaObject::invokeMethod(dbWorker, &TRUSDBWorker::shutdown, Qt::BlockingQueuedConnection);
        dbThread.quit();
        dbThread.wait();
    });

    dbThread.start();

    return app.exec();
}