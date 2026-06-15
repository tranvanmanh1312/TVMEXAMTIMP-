#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QHash>
#include <QSet>
#include <QDebug>

class MyTcpServer : public QObject
{
    Q_OBJECT

public:
    explicit MyTcpServer(quint16 port = 8080, int maxClients = 6, QObject *parent = nullptr);
    ~MyTcpServer();

private slots:
    void slotNewConnection();
    void slotReadyRead();
    void slotClientDisconnected();

private:
    QTcpServer *mTcpServer;
    QList<QTcpSocket*> clients;
    QHash<QTcpSocket*, int> bids;
    QSet<QTcpSocket*> waitingAnswer;

    quint16 serverPort;
    int maxClientCount;

    void sendToClient(QTcpSocket *client, const QString &message);
    void broadcast(const QString &message);
    void broadcastClientCount();
    void finishAuction();
    QString clientName(QTcpSocket *client) const;
};

#endif // MYTCPSERVER_H