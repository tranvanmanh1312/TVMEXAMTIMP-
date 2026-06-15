#include "mytcpserver.h"

MyTcpServer::MyTcpServer(quint16 port, int maxClients, QObject *parent)
    : QObject(parent),
      mTcpServer(new QTcpServer(this)),
      serverPort(port),
      maxClientCount(maxClients)
{
    connect(mTcpServer, &QTcpServer::newConnection, this, &MyTcpServer::slotNewConnection);
    if (!mTcpServer->listen(QHostAddress::Any, serverPort)) {
        qDebug() << "Server is not started:" << mTcpServer->errorString();
    } else {
        qDebug() << "Server is started on port" << serverPort;
        qDebug() << "Maximum clients:" << maxClientCount;
    }
}

MyTcpServer::~MyTcpServer()
{
    for (QTcpSocket *client : clients) {
        client->disconnectFromHost();
    }
    mTcpServer->close();
}

QString MyTcpServer::clientName(QTcpSocket *client) const
{
    return QString("Client_%1").arg(client->socketDescriptor());
}

void MyTcpServer::sendToClient(QTcpSocket *client, const QString &message)
{
    if (client && client->state() == QAbstractSocket::ConnectedState) {
        client->write(message.toUtf8());
        client->write("\r\n");
        client->flush();
    }
}

void MyTcpServer::broadcast(const QString &message)
{
    for (QTcpSocket *client : clients) {
        sendToClient(client, message);
    }

    qDebug() << message;
}

void MyTcpServer::broadcastClientCount()
{
    QString message = QString("Current number of connected clients: %1")
                          .arg(clients.size());
    broadcast(message);
}

void MyTcpServer::slotNewConnection()
{
    QTcpSocket *client = mTcpServer->nextPendingConnection();
    if (clients.size() >= maxClientCount) {
        sendToClient(client, "Server is busy. Please connect later.");
        qDebug() << "Extra client rejected:" << client->peerAddress().toString();
        client->disconnectFromHost();
        client->deleteLater();
        return;
    }

    clients.append(client);
    bids.insert(client, 0);

    connect(client, &QTcpSocket::readyRead,
            this, &MyTcpServer::slotReadyRead);
    connect(client, &QTcpSocket::disconnected,
            this, &MyTcpServer::slotClientDisconnected);

    sendToClient(client, "Welcome to the auction server!");
    sendToClient(client, "Send your bid as a number, for example: 100");
    sendToClient(client, "To finish auction, send: END");

    qDebug() << clientName(client) << "connected";
    broadcastClientCount();
}

void MyTcpServer::slotReadyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {
        return;
    }

    QString message = QString::fromUtf8(client->readAll()).trimmed();
    QString lowerMessage = message.toLower();

    qDebug() << clientName(client) << "sent:" << message;

    if (waitingAnswer.contains(client)) {
        if (lowerMessage == "no" || lowerMessage == "нет" || lowerMessage == "n") {
            sendToClient(client, "You answered no. Connection will be closed.");
            qDebug() << clientName(client) << "left auction";
            waitingAnswer.remove(client);
            client->disconnectFromHost();
            return;
        }

        if (lowerMessage == "yes" || lowerMessage == "да" || lowerMessage == "y") {
            waitingAnswer.remove(client);
            bids[client] = 0;
            sendToClient(client, "You continue participation in the next auction.");
            broadcastClientCount();
            return;
        }

        sendToClient(client, "Please answer yes/no or да/нет.");
        return;
    }

    if (lowerMessage == "end" || lowerMessage == "finish" || lowerMessage == "stop") {
        finishAuction();
        return;
    }

    bool ok = false;
    int bid = message.toInt(&ok);
    if (!ok || bid <= 0) {
        sendToClient(client, "Incorrect bid. Send positive number or END.");
        return;
    }

    bids[client] = bid;

    QString bidMessage = QString("%1 made a bid: %2")
                             .arg(clientName(client))
                             .arg(bid);

    broadcast(bidMessage);
}

void MyTcpServer::finishAuction()
{
    if (clients.isEmpty()) {
        qDebug() << "Auction finished: no clients";
        return;
    }

    QTcpSocket *winner = nullptr;
    int maxBid = -1;

    for (QTcpSocket *client : clients) {
        int bid = bids.value(client, 0);
        if (bid > maxBid) {
            maxBid = bid;
            winner = client;
        }
    }

    if (winner == nullptr || maxBid <= 0) {
        broadcast("Auction finished. There are no valid bids.");
    } else {
        QString result = QString("Auction finished. Winner: %1, bid: %2")
                             .arg(clientName(winner))
                             .arg(maxBid);

        broadcast(result);
    }

    broadcast("Будете участвовать еще? Ответьте yes/no или да/нет.");
    waitingAnswer.clear();

    for (QTcpSocket *client : clients) {
        waitingAnswer.insert(client);
    }
}

void MyTcpServer::slotClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());

    if (!client) {
        return;
    }

    qDebug() << clientName(client) << "disconnected";

    clients.removeAll(client);
    bids.remove(client);
    waitingAnswer.remove(client);
    client->deleteLater();
    broadcastClientCount();
}