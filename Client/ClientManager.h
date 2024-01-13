#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <QObject>
#include <string>
#include "../Common/networkCodec.h"
#include "../Common/verify.pb.h"

class QQmlApplicationEngine;
class QTcpSocket;
class ClientManager : public QObject
{
    Q_OBJECT
    
    enum EnterState
    {
        LoginState_None,
        LoginState_Register,
        LoginState_GetToken,
        LoginState_Verify,
        LoginState_Login,
        LoginState_Max
    };
    
signals:
    void registerCallback(bool success);
    void loginCallback(bool success);
    void quitCallback(bool success);
    
public:
    explicit ClientManager(QObject *parent = nullptr);
    void Initialize(void);
    void setQmlEngine(QQmlApplicationEngine *engine);
    
public slots:
    void handleRegister(const QString &username, const QString &password);
    void handleLogin(const QString &username, const QString &password);
    void handleQuit(void);
    void logOut(void);
    
private slots:
    void connected();
    void disconnected();
    
private:
    void HandleMessage(QTcpSocket *socket, unsigned int message_id, const char *data, int len);
    
    void SendRegisterReq(QTcpSocket* socket);
    void HandleRegisterAck(verify::RegisterAck ack);
    
    void SendGetTokenReq(QTcpSocket* socket);
    void HandleGetTokenAck(QTcpSocket* socket, verify::GetTokenAck ack);
    
    void SendLoginReq(QTcpSocket* socket);
    void HandleLoginAck(verify::LoginAck ack);
    
    void SendQuitReq(QTcpSocket* socket);
    void HandleQuitAck(verify::QuitAck ack);


private:
    QQmlApplicationEngine* m_qml_engine;
    QTcpSocket *m_tcp_client;
    QTcpSocket* m_curLoginSocket;
    
    QString m_user_name;
    QString m_password;
    
    EnterState m_state = { LoginState_None };
    
    std::string m_token;
    
    NetworkCodec m_net_codec;

};

#endif // CLIENTMANAGER_H
