#include "telnetreader.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTcpSocket>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    const QString hostName {};
    const quint16 port {};
    const QByteArray command {};

    TelnetReader telnet(hostName, port);

    QObject::connect(
        &telnet,
        &TelnetReader::connected,
        &telnet,
        [&telnet, command]
        {
            qInfo() << "Connected";
            telnet.writeLine(command);
        });

    QObject::connect(
        &telnet,
        &TelnetReader::disconnected,
        []
        {
            qInfo() << "Disconnected";
        });

    QObject::connect(
        &telnet,
        &TelnetReader::lineReceived,
        [](const QByteArray& line)
        {
            qInfo().noquote() << QString::fromUtf8(line);
        });

    QObject::connect(
        &telnet,
        &TelnetReader::errorOcurred,
        [](const QString& error)
        {
            qWarning() << "Telnet error:" << error;
        });

    telnet.connectToHost();

    return app.exec();
}