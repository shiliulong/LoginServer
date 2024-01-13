#ifndef VERIFYMANAGER_H
#define VERIFYMANAGER_H

#include <QObject>
#include "../Common/networkCodec.h"
#include "../Common/verify.pb.h"

class QTcpSocket;
class QTcpServer;
class VerifyManager : public QObject
{
    Q_OBJECT
public:
    VerifyManager(QObject *parent = nullptr);
    
    void Initialize( void );
    
private slots:
    void NewConnection( void );
    void Disconnected( void );
    
private:
    void HandleMessage(QTcpSocket* socket, unsigned int message_id, const char* data, int len);
    
    void HandleRegisterReq(QTcpSocket* socket, verify::RegisterReq& req);
    void SendRegisterAck(QTcpSocket* socket, bool is_ok, const std::string& error_info);
    
    void HandleGetTokenReq(QTcpSocket* socket, verify::GetTokenReq& req);
    void SendGetTokenAck(QTcpSocket* socket, bool is_ok, const std::string& name);
    
    void HandleVerifyReq(QTcpSocket* socket, verify::VerifyReq& req);
    void SendVerifyAck(QTcpSocket* socket, bool is_ok, const std::string& token);
    
private:
    QTcpServer *m_tcp_server;
    NetworkCodec m_net_codec;
};

#endif // VERIFYMANAGER_H
