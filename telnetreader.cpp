#include "telnetreader.h"
#include <QObject>
#include <QTcpSocket>

TelnetReader::TelnetReader(QString hostName, quint16 port, QObject *parent)
    : QObject(parent), m_hostName(hostName), m_port(port) {
    QObject::connect(
        &m_socket,
        &QTcpSocket::connected,
        this,
        [this] {emit connected();}
    );

    QObject::connect(
        &m_socket,
        &QTcpSocket::disconnected,
        this,
        &TelnetReader::disconnected
    );

    QObject::connect(
        &m_socket,
        &QTcpSocket::readyRead,
        this,
        &TelnetReader::onReadyRead
    );

    QObject::connect(
        &m_socket,
        &QTcpSocket::errorOccurred,
        this,
        [this] (QAbstractSocket::SocketError) {emit errorOcurred(m_socket.errorString());}
    );
};

// PUBLIC
void TelnetReader::connectToHost() {
    if (m_socket.state() != QAbstractSocket::UnconnectedState) return;

    m_lineBuffer.clear();

    m_telnetState = TelnetState::Data;
    m_telnetCommand = 0;
    m_pendingCR = false;

    m_socket.connectToHost(m_hostName, m_port);
}

void TelnetReader::disconnectFromHost() {
    if (m_socket.state() == QAbstractSocket::UnconnectedState) return;

    m_socket.disconnectFromHost();
}

void TelnetReader::write(const QByteArray& data) {
    m_socket.write(data);
}

void TelnetReader::writeLine(const QByteArray& data) {
    m_socket.write(data + "\r\n");
}

bool TelnetReader::isConnected() const {return (m_socket.state() == QAbstractSocket::ConnectedState);}

// PRIVATE SLOTS
void TelnetReader::onReadyRead() {processIncoming(m_socket.readAll());}

// PRIVATE METHODS
void TelnetReader::processIncoming(const QByteArray& data) {
    for (const char value: data) {
        const unsigned char byte = static_cast<unsigned char>(value);

        switch (m_telnetState) {
            case TelnetState::Data:
                if (byte == TelnetCode::IAC) m_telnetState = TelnetState::IAC;
                else processApplicationByte(value);
                break;
            case TelnetState::IAC:
                switch (byte) {
                    case TelnetCode::IAC: // IAC IAC represents a literal 0xFF data byte.
                        processApplicationByte(static_cast<char>(TelnetCode::IAC));
                        m_telnetState = TelnetState::Data;
                        break;
                    case TelnetCode::WILL:
                    case TelnetCode::WONT:
                    case TelnetCode::DO:
                    case TelnetCode::DONT:
                        m_telnetCommand = byte;
                        m_telnetState = TelnetState::Negotiation;
                        break;
                    case TelnetCode::SB:
                        m_telnetState = TelnetState::Subnegotiation;
                        break;

                    case TelnetCode::SE:
                    case TelnetCode::NOP:
                    case TelnetCode::DataMark:
                    case TelnetCode::Break:
                    case TelnetCode::InterruptProcess:
                    case TelnetCode::AbortOutput:
                    case TelnetCode::AreYouThere:
                    case TelnetCode::EraseCharacter:
                    case TelnetCode::EraseLine:
                    case TelnetCode::GoAhead:
                        m_telnetState = TelnetState::Data;
                        break;

                    default:
                        m_telnetState = TelnetState::Data;
                        break;
                }

                    break;

            case TelnetState::Negotiation:
                rejectTelnetOption(m_telnetCommand, byte);
                m_telnetCommand = 0;
                m_telnetState = TelnetState::Data;
                break;

            case TelnetState::Subnegotiation:
                if (byte == TelnetCode::IAC)
                    m_telnetState = TelnetState::SubnegotiationIAC;

                break;

            case TelnetState::SubnegotiationIAC:
                if (byte == TelnetCode::SE) {
                    m_telnetState = TelnetState::Data;
                } else if (byte == TelnetCode::IAC) {
                    // Escaped IAC inside subnegotiation data.
                    m_telnetState = TelnetState::Subnegotiation;
                } else {
                    // Still inside the subnegotiation sequence.
                    m_telnetState = TelnetState::Subnegotiation;
                }

                break;
        }
    }
};
void TelnetReader::processApplicationByte (char byte) {
    if (m_pendingCR) {
        m_pendingCR = false;

        if (byte == '\n') {
            emit lineReceived(m_lineBuffer);
            m_lineBuffer.clear();
            return;
        }

        if (byte == '\0') {
            m_lineBuffer.append('\r');
            return;
        }

        m_lineBuffer.append('\r');
    }

    if (byte == '\r') {
        m_pendingCR = true;
        return;
    }

    if (byte == '\n') {
        emit lineReceived(m_lineBuffer);
        m_lineBuffer.clear();
        return;
    }

    m_lineBuffer.append(byte);
};
void TelnetReader::rejectTelnetOption(unsigned char command, unsigned char option) {
    unsigned char response;

    switch (command) {
    case TelnetCode::DO:
        response = TelnetCode::WONT;
        break;

    case TelnetCode::WILL:
        response = TelnetCode::DONT;
        break;

    case TelnetCode::DONT:
    case TelnetCode::WONT:
        return;

    default:
        return;
    }

    QByteArray reply;
    reply.reserve(3);

    reply.append(static_cast<char>(TelnetCode::IAC));
    reply.append(static_cast<char>(response));
    reply.append(static_cast<char>(option));

    m_socket.write(reply);
};