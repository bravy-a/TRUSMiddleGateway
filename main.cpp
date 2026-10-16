#include <unordered_set>
#include <vector>
#include <type_traits>
#include <chrono>
#include <memory>
#include <utility>
#include <cstdint>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThread>
#include <QTimeZone>
#include <QTimer>
#include <QVariant>
#include <QSettings>
#include "datamodels.h"
#include "marketinfoparser.h"
#include "marketinfosummarymodel.h"
#include "telnetreader.h"
#include "trusdbworker.h"

using namespace std::chrono_literals;

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    QSettings settings("config.ini", QSettings::IniFormat);

    settings.beginGroup("TRUS");
    const QString hostName = settings.value("hostName").toString();
    const quint16 port = settings.value("port").value<quint16>();
    const QByteArray command = settings.value("command").toByteArray();
    settings.endGroup();

    settings.beginGroup("DATABASE");
    DBPayload dbPayload;
    dbPayload.driver = settings.value("driver").toString();
    dbPayload.server = settings.value("server").toString();
    dbPayload.dbName = settings.value("dbName").toString();
    dbPayload.userName = settings.value("userName").toString();
    dbPayload.userPassword = settings.value("userPassword").toString();
    settings.endGroup();

    settings.beginGroup("SONA_DATABASE");
    DBPayload dbPayloadSona;
    dbPayloadSona.driver = settings.value("driver").toString();
    dbPayloadSona.server = settings.value("server").toString();
    dbPayloadSona.dbName = settings.value("dbName").toString();
    dbPayloadSona.userName = settings.value("userName").toString();
    dbPayloadSona.userPassword = settings.value("userPassword").toString();
    settings.endGroup();

    settings.beginGroup("LOGIC");
    const int uploadInterval {settings.value("uploadInterval").toInt()};
    settings.endGroup();

    if (dbPayload.driver.isEmpty() || dbPayload.server.isEmpty() || dbPayload.dbName.isEmpty() || dbPayload.userName.isEmpty() || dbPayload.userPassword.isEmpty()) {
        qCritical() << "TRUS database configuration is incomplete";
        return 1;
    }

    if (dbPayloadSona.driver.isEmpty() || dbPayloadSona.server.isEmpty() || dbPayloadSona.dbName.isEmpty() || dbPayloadSona.userName.isEmpty() || dbPayloadSona.userPassword.isEmpty()) {
        qCritical() << "Sona database configuration is incomplete";
        return 1;
    }

    const QDate qTradingDate {QDateTime::currentDateTimeUtc().toTimeZone(QTimeZone(defaultTimezone)).date()};
    const std::chrono::year_month_day tradingDate {
        std::chrono::year {qTradingDate.year()},
        std::chrono::month {static_cast<unsigned>(qTradingDate.month())},
        std::chrono::day {static_cast<unsigned>(qTradingDate.day())}
    };

    MarketInfoParser marketInfoParser {tradingDate};
    PriceData priceData;
    MarketInfoSummaryModel marketInfoSummary;

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("marketInfoSummary"), QVariant::fromValue(&marketInfoSummary)}
    });

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] {
        QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.loadFromModule("TRUSMiddleGateway", "Main");

    // Spawn DB worker on a separate thread
    QThread dbThread;
    auto* dbWorker = new TRUSDBWorker(std::move(dbPayload), QStringLiteral("TRUS_DB"));
    dbWorker->moveToThread(&dbThread);

    QObject::connect(&dbThread, &QThread::started, dbWorker, &TRUSDBWorker::initialize);
    QObject::connect(&dbThread, &QThread::finished, dbWorker, &QObject::deleteLater);

    // Spawn Sona DB worker on another thread
    QThread dbThreadSona;
    auto* dbWorkerSona = new TRUSDBWorker(std::move(dbPayloadSona), QStringLiteral("SONA_DB"));
    dbWorkerSona->moveToThread(&dbThreadSona);

    QObject::connect(&dbThreadSona, &QThread::started, dbWorkerSona, &TRUSDBWorker::initialize);
    QObject::connect(&dbThreadSona, &QThread::finished, dbWorkerSona, &QObject::deleteLater);

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

    QObject::connect(telnet.get(), &TelnetReader::disconnected, [] { qInfo() << "Disconnected from TRUS"; });
    QObject::connect(telnet.get(), &TelnetReader::errorOcurred, [](const QString& error) { qWarning() << "Telnet error:" << error; });

    // Track changes to consider what has to be updated.
    std::unordered_set<QString> updatedStocks, uploadingStocks;

    bool uploadInFlight {false};
    bool uploadInFlightSona {false};

    // Helper: clear uploadingStocks when all worker are finished
    auto maybeClearUploading = [&] {
        if (!uploadInFlight && !uploadInFlightSona)
            uploadingStocks.clear();
    };

    std::uint64_t parsedLines {0};

    const auto makeRows = [&priceData](const std::unordered_set<QString>& stocks) {
        auto snapshot = priceData.GetPriceDataSnapshot();
        std::vector<PriceDataRow> rows;
        rows.reserve(stocks.size());

        for (const QString& stockCode : stocks)
            if (auto it = snapshot.find(stockCode); it != snapshot.end()) rows.emplace_back(std::move(it->second));

        return rows;
    };

    // Upload function to KGI DB
    const auto uploadRows = [](std::vector<PriceDataRow> rows, Qt::ConnectionType connectionType, TRUSDBWorker* dbWorker) {
        QMetaObject::invokeMethod(dbWorker, [dbWorker, rows = std::move(rows)]() mutable {
            dbWorker->uploadPriceData(std::move(rows));
        }, connectionType);
    };

    // Main parser of each msg from TRUS.
    // Replies server heartbeat when received.
    QObject::connect(telnet.get(), &TelnetReader::lineReceived, &app, [&](const QByteArray& line) {
        if (line.isEmpty()) return;

        const char messageType {line.front()};

        // Record login confirmation
        if (messageType == 'R') {
            marketInfoSummary.recordMessage(messageType);
            return;
        }

        // Record heartbeat from server and reply with client heartbeat
        if (messageType == 'S') {
            marketInfoSummary.recordServerHeartbeat();

            if (telnet->isConnected()) {
                telnet->writeLine("C|");
                marketInfoSummary.recordClientHeartbeat();
            }

            return;
        }

        // Parse message and add to counter
        try {
            const ParsedDataTypes parsedData {marketInfoParser.ParseMessage(line)};
            marketInfoSummary.recordMessage(messageType);

            std::visit([&](const auto& data) {
                using T = std::decay_t<decltype(data)>;

                if constexpr (std::is_same_v<T, std::monostate>) {return;}
                else {
                    if (data.marketCode != MarketCode::RG) return;

                    if constexpr (std::is_same_v<T, InitialStockInfo>) priceData.UpdateInitialStockInfo(data);
                    else if constexpr (std::is_same_v<T, StockOrderBook>) priceData.UpdateStockOrderBook(data);
                    else if constexpr (std::is_same_v<T, StockTradeBook>) priceData.UpdateStockTradeBook(data);
                    else if constexpr (std::is_same_v<T, Trade>) priceData.UpdateTrade(data);
                    else if constexpr (std::is_same_v<T, IndicativeEquilibriumData>) {
                        if (messageType == 'e') priceData.UpdateIndicativeEquilibriumOpeningData(data);
                        else if (messageType == 'f') priceData.UpdateIndicativeEquilibriumClosingData(data);
                    }

                    updatedStocks.insert(data.stockCode);
                }
            }, parsedData);

            ++parsedLines;
        }
        catch (const std::exception& e) {
            qWarning() << "Parse error:" << e.what() << "Message:" << line;
        }
    });

    // Batch upload to DB
    QTimer uploadTimer;
    uploadTimer.setInterval(uploadInterval);

    QObject::connect(&uploadTimer, &QTimer::timeout, &app, [&] {
        if (uploadInFlight || uploadInFlightSona || updatedStocks.empty())
            return;

        auto rows = makeRows(updatedStocks);
        if (rows.empty()) {
            updatedStocks.clear();
            return;
        }

        uploadingStocks.clear();
        uploadingStocks.swap(updatedStocks);
        uploadInFlight = true;
        uploadInFlightSona = true;

        uploadRows(rows, Qt::QueuedConnection, dbWorker);
        uploadRows(std::move(rows), Qt::QueuedConnection, dbWorkerSona);
    });

    QObject::connect(dbWorker, &TRUSDBWorker::uploadCompleted, &app, [&](qsizetype rowCount) {
        uploadInFlight = false;
        maybeClearUploading();
        qInfo() << "Uploaded rows:" << rowCount;
    });

    QObject::connect(dbWorker, &TRUSDBWorker::errorOccurred, &app, [&](const QString& error) {
        if (uploadInFlight) {
            updatedStocks.insert(uploadingStocks.begin(), uploadingStocks.end());
            uploadInFlight = false;
        }
        maybeClearUploading();
        qWarning() << "Database error:" << error;
    });

    // Upload-Error for dbWorkerSona
    QObject::connect(dbWorkerSona, &TRUSDBWorker::uploadCompleted, &app, [&](qsizetype rowCount) {
        uploadInFlightSona = false;
        maybeClearUploading(); // Clear when all worker is finised
        qInfo() << "(Sona) Uploaded rows:" << rowCount;
    });

    QObject::connect(dbWorkerSona, &TRUSDBWorker::errorOccurred, &app, [&](const QString& error) {
        if (uploadInFlightSona) {
            updatedStocks.insert(uploadingStocks.begin(), uploadingStocks.end());
            uploadInFlightSona = false;
        }
        maybeClearUploading();
        qWarning() << "(Sona) Database error:" << error;
    });

    // TRUS data feed not started until the KGI DB and SONA DB connection is established

    bool initialized {false};
    bool initializedSona {false};
    bool truncated {false};
    bool truncatedSona {false};
    bool feedStarted {false};


    // Truncate function to KGI DB
    const auto truncateTable = [](Qt::ConnectionType connectionType, TRUSDBWorker* dbWorker) {
        QMetaObject::invokeMethod(dbWorker, [dbWorker]() mutable {
            dbWorker->truncatePriceTable();
        }, connectionType);
    };

    const auto startFeedIfReady = [&] {
        if (initialized && initializedSona && truncated && truncatedSona && !feedStarted){
            feedStarted = true;
            uploadTimer.start();
            telnet->connectToHost();
        }
    };

    QObject::connect(dbWorker, &TRUSDBWorker::tableTruncated, &app, [&]() {
        qInfo() << "Table Truncated.";
        truncated = true;
        startFeedIfReady();
    });

    QObject::connect(dbWorkerSona, &TRUSDBWorker::tableTruncated, &app, [&]() {
        qInfo() << "(Sona) Table Truncated.";
        truncatedSona = true;
        startFeedIfReady();
    });

    QObject::connect(dbWorker, &TRUSDBWorker::initialized, &app, [&] {
        initialized = true;
        truncateTable(Qt::QueuedConnection, dbWorker);
    });

    QObject::connect(dbWorkerSona, &TRUSDBWorker::initialized, &app, [&] {
        initializedSona = true;
        truncateTable(Qt::QueuedConnection, dbWorkerSona);
    });

    // Clean shutdown
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [&] {
        telnet.reset();

        // Flush anything that changed after the previous batch was started
        if (!updatedStocks.empty()) {
            auto rows = makeRows(updatedStocks);
            if (!rows.empty()) {
                uploadRows(rows, Qt::BlockingQueuedConnection, dbWorker);
                uploadRows(std::move(rows), Qt::BlockingQueuedConnection, dbWorkerSona);
            }
        }

        QMetaObject::invokeMethod(dbWorker, &TRUSDBWorker::shutdown, Qt::BlockingQueuedConnection);
        dbThread.quit();
        dbThread.wait();

        QMetaObject::invokeMethod(dbWorkerSona, &TRUSDBWorker::shutdown, Qt::BlockingQueuedConnection);
        dbThreadSona.quit();
        dbThreadSona.wait();
    });

    dbThread.start();
    dbThreadSona.start();

    return app.exec();
}