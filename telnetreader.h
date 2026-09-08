#pragma once

#include <QObject>
#include <QTcpSocket>
#include <functional>

namespace TelnetCode {
constexpr unsigned char SE {240};
constexpr unsigned char NOP {241};
constexpr unsigned char DataMark {242};
constexpr unsigned char Break {243};
constexpr unsigned char InterruptProcess {244};
constexpr unsigned char AbortOutput {245};
constexpr unsigned char AreYouThere {246};
constexpr unsigned char EraseCharacter {247};
constexpr unsigned char EraseLine {248};
constexpr unsigned char GoAhead {249};
constexpr unsigned char SB {250};
constexpr unsigned char WILL {251};
constexpr unsigned char WONT {252};
constexpr unsigned char DO {253};
constexpr unsigned char DONT {254};
constexpr unsigned char IAC	{255};
}

class TelnetReader : public QObject
{
    Q_OBJECT
public:
    explicit TelnetReader(QString hostName, quint16 port, std::function<void(TelnetReader&)> destructSequence = {}, QObject *parent = nullptr);
    ~TelnetReader();
    void connectToHost();
    void disconnectFromHost();

    void write(const QByteArray& data);
    void writeLine(const QByteArray& line);

    [[nodiscard]] bool isConnected() const;

signals:
    void connected();
    void disconnected();

    void lineReceived(const QByteArray& line);
    void errorOcurred(const QString& error);

private slots:
    void onReadyRead();

private:
    enum class TelnetState {
        Data,
        IAC,
        Negotiation,
        Subnegotiation,
        SubnegotiationIAC
    };


    void processIncoming(const QByteArray& data);
    void processApplicationByte (char byte);
    void rejectTelnetOption(unsigned char command, unsigned char option);

    QString m_hostName;
    quint16 m_port;

    QTcpSocket m_socket;

    QByteArray m_lineBuffer;

    TelnetState m_telnetState = TelnetState::Data;
    unsigned char m_telnetCommand = 0;

    bool m_pendingCR = false;
    bool m_manualDisconnect = false;
    std::function<void(TelnetReader&)> m_destructSequence;
};