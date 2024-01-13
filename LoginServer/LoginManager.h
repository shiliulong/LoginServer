#ifndef LOGINMANAGER_H
#define LOGINMANAGER_H

#include <QObject>
#include <string>

#include "../Common/networkCodec.h"
#include "../Common/verify.pb.h"


class QTcpSocket;
class QTcpServer;

class LoginManager : public QObject
{
    Q_OBJECT
public:
    explicit LoginManager(QObject *parent = nullptr);
    void Initialize(void);
    
private slots:
    void NewConnection(void);
    void Disconnected(void);
    
    void ClientConnection(void);
    void ClientDisconnected(void);
    
private:
    void HandleMessage(QTcpSocket *socket, unsigned int message_id, const char *data, int len);
    
    void HandleLoginReq(QTcpSocket *socket, verify::LoginReq& req);
    void SendLoginAck(bool is_ok, const std::string &token);
    
    void HandleVerifyAck(verify::VerifyAck &ack);
    void SendVerifyReq(QTcpSocket *socket);
    
    void HandleQuitReq(QTcpSocket *socket, verify::QuitReq &req);
    void SendQuitAck(QTcpSocket *socket);
    
private:
    QTcpServer *m_tcp_server;
    QTcpSocket *m_tcp_client;
    
    std::string m_token;
    NetworkCodec m_net_codec;
    
    std::map<std::string, QTcpSocket*> m_connected_clients;
    
    bool m_is_verifyReq_server_connected { false };
};

#endif // LOGINMANAGER_H
