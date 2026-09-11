#pragma once
#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QString>
#include <QVariant>
#include <QtQml/qqmlregistration.h>
#include <QTimeZone>
#include <array>
#include <cstddef>
#include <cstdint>

// TO DO: Move to config
static const QTimeZone defaultTimezone {"Asia/Jakarta"};

enum class SummaryRow : std::size_t {
    LoginReply,
    InitialStockInfo,
    Trade,
    UpdateStockOrderBook,
    UpdateStockTradeBook,
    UpdateStockIEPIEV,
    UpdateStockIEPIEVClosing,
    Count
};

class MarketInfoSummaryModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("MarketInfoSummaryModel is created by C++")

    // Keep track of server and client heartbeats.
    Q_PROPERTY(QString lastServerHeartbeat READ lastServerHeartbeat NOTIFY lastServerHeartbeatChanged)
    Q_PROPERTY(QString lastClientHeartbeat READ lastClientHeartbeat NOTIFY lastClientHeartbeatChanged)

public:
    enum Role {
        LabelRole = Qt::UserRole + 1,
        CountRole
    };
    Q_ENUM(Role)

    explicit MarketInfoSummaryModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] const QString& lastServerHeartbeat() const noexcept {return m_lastServerHeartbeat;}
    [[nodiscard]] const QString& lastClientHeartbeat() const noexcept {return m_lastClientHeartbeat;}

    void recordMessage(char messageType);
    void recordServerHeartbeat();
    void recordClientHeartbeat();

signals:
    void lastServerHeartbeatChanged();
    void lastClientHeartbeatChanged();

private:
    struct Row {
        QString label;
        std::uint64_t count {0};
    };

    static constexpr std::size_t RowCount {static_cast<std::size_t>(SummaryRow::Count)};

    void increment(SummaryRow row);
    [[nodiscard]] static QString currentTime(QTimeZone timeZone = defaultTimezone);

    std::array<Row, RowCount> m_rows;
    QString m_lastServerHeartbeat;
    QString m_lastClientHeartbeat;
};