#include "marketinfosummarymodel.h"
#include <QDateTime>
#include <QTimeZone>

MarketInfoSummaryModel::MarketInfoSummaryModel(QObject* parent)
    : QAbstractListModel(parent),
    m_rows {{
        {QStringLiteral("Login Reply"), 0},
        {QStringLiteral("Initial Stock Info"), 0},
        {QStringLiteral("Trade"), 0},
        {QStringLiteral("Update Stock Order Book"), 0},
        {QStringLiteral("Update Stock Trade Book"), 0},
        {QStringLiteral("Update Stock IEP IEV"), 0},
        {QStringLiteral("Update Stock IEP IEV Closing"), 0}
    }}
{}

int MarketInfoSummaryModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant MarketInfoSummaryModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) return {};

    const Row& row = m_rows.at(static_cast<std::size_t>(index.row()));

    switch (role) {
    case LabelRole:
        return row.label;
    case CountRole:
        return QVariant::fromValue<qulonglong>(row.count);
    default:
        return {};
    }
}

QHash<int, QByteArray> MarketInfoSummaryModel::roleNames() const {
    return {
        {LabelRole, "label"},
        {CountRole, "count"}
    };
}

void MarketInfoSummaryModel::recordMessage(char messageType) {
    switch (messageType) {
    case 'R': increment(SummaryRow::LoginReply); break;
    case 'y': increment(SummaryRow::InitialStockInfo); break;
    case 't': increment(SummaryRow::Trade); break;
    case 'v': increment(SummaryRow::UpdateStockOrderBook); break;
    case 'x': increment(SummaryRow::UpdateStockTradeBook); break;
    case 'e': increment(SummaryRow::UpdateStockIEPIEV); break;
    case 'f': increment(SummaryRow::UpdateStockIEPIEVClosing); break;
    default: break;
    }
}

void MarketInfoSummaryModel::recordServerHeartbeat() {
    m_lastServerHeartbeat = currentTime();
    emit lastServerHeartbeatChanged();
}

void MarketInfoSummaryModel::recordClientHeartbeat() {
    m_lastClientHeartbeat = currentTime();
    emit lastClientHeartbeatChanged();
}

void MarketInfoSummaryModel::increment(SummaryRow row) {
    const auto rowNumber = static_cast<std::size_t>(row);
    ++m_rows.at(rowNumber).count;

    const QModelIndex changedIndex = index(static_cast<int>(rowNumber), 0);
    emit dataChanged(changedIndex, changedIndex, {CountRole});
}

QString MarketInfoSummaryModel::currentTime(QTimeZone timeZone) {
    static const QTimeZone jakartaTimeZone {"Asia/Jakarta"};
    return QDateTime::currentDateTimeUtc().toTimeZone(jakartaTimeZone).toString(QStringLiteral("HH:mm:ss 'WIB'"));
}
