#include "LoginManager.h"
#include "../Common/comDefinition.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QtEndian>
#include <functional>

LoginManager::LoginManager(QObject *parent) : QObject{parent}, m_net_codec(parent)
{
    qDebug() << "开始LoginServer的构造函数";
    m_tcp_server = new QTcpServer(this);
    
    connect(m_tcp_server, &QTcpServer::newConnection, this, &LoginManager::NewConnection);
    
    if(!m_tcp_server->listen(QHostAddress::SpecialAddress::LocalHost, 6060))
    {
        qDebug() << "login server 启动失败";
    }
    else
    {
        qDebug() << "login server 启动成功";
    }
    
    m_tcp_client = new QTcpSocket(this);
    
    connect(m_tcp_client, &QTcpSocket::connected, this, &LoginManager::ClientConnection);
    connect(m_tcp_client, &QTcpSocket::disconnected, this, &LoginManager::ClientDisconnected);
}

void LoginManager::Initialize()
{
    m_net_codec.SetMessageCallback(std::bind(&LoginManager::HandleMessage, this
                                             , std::placeholders::_1, std::placeholders::_2
                                             , std::placeholders::_3, std::placeholders::_4));
    
    qDebug() << "在LoginManager::Initialize 调用";
    m_tcp_client->connectToHost("localhost", 5060);
}

void LoginManager::NewConnection()
{
    QTcpSocket *socket = m_tcp_server->nextPendingConnection();
    
    connect(socket, &QTcpSocket::disconnected, this, &LoginManager::Disconnected);
    connect(socket, &QTcpSocket::readyRead, &m_net_codec, &NetworkCodec::ReadyRead);
    
    m_net_codec.RegisterSocket(socket);
    
    qDebug() << "收到新的客户链接 IP:" << socket->peerAddress().toString() << "Port:" << socket->peerPort(); 
}

void LoginManager::Disconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    qDebug() << "LoginServer 连接断开";
    
    m_net_codec.UnRegisterSocket(socket);
    
    socket->deleteLater();
}

void LoginManager::ClientConnection()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    qDebug() << "LoginServer 连接 VerifyServer 成功,socket:" << socket << "m_tcp_clent:" << m_tcp_client;
    m_net_codec.RegisterSocket(socket);
    connect(socket, &QTcpSocket::readyRead, &m_net_codec, &NetworkCodec::ReadyRead);
    
    m_is_verifyReq_server_connected = true;
}

void LoginManager::ClientDisconnected()
{
    QTcpSocket *socket = (QTcpSocket*)sender();
    qDebug() << "连接断开";
    
    m_net_codec.UnRegisterSocket(socket);
    m_is_verifyReq_server_connected = false;
}

void LoginManager::HandleMessage(QTcpSocket *socket, unsigned int message_id, const char *data, int len)
{
    qDebug() << "进入到Login::Manager::HandleMessage, m_message_id" << message_id << " len:" << len;
    
    switch(message_id)
    {
        case MessageId::kLoginReq:
        {
            verify::LoginReq loginReq;
            loginReq.ParseFromString(data);
            HandleLoginReq(socket, loginReq);
            break;
        }
        case MessageId::kVerifyAck:
        {
            verify::VerifyAck verifyAck;
            verifyAck.ParseFromString(data);
            HandleVerifyAck(verifyAck);
        }
        case MessageId::kQuitReq:
        {
            verify::QuitReq quitReq;
            quitReq.ParseFromString(data);
            HandleQuitReq(socket, quitReq);
            break;
        }
        default:
            break;
    }
}

void LoginManager::HandleLoginReq(QTcpSocket *socket, verify::LoginReq &req)
{
    m_token = req.token();
    
    if(m_connected_clients.find(m_token) == m_connected_clients.end())
    {
        m_connected_clients[m_token] = socket;
    }
    
    qDebug() << "trace log 10 LoginManager::HandleLoginServerReq" << " token:" << m_token.c_str();
    
    if(m_is_verifyReq_server_connected == true)
    {
        this->SendVerifyReq(m_tcp_client);
    }
    else
    {
        this->SendLoginAck(false, m_token);
    }
}

void LoginManager::SendLoginAck(bool is_ok, const std::string &token)
{
    qDebug() << "trace log 15 LoginManager::SendLoginServerAck, token:" << token.c_str();
    verify::LoginAck loginAck;
    loginAck.set_is_ok(is_ok);
    std::string data;
    loginAck.SerializeToString(&data);
    
    auto it = m_connected_clients.find(token);
    if(it != m_connected_clients.end())
    {
        qDebug() << "trace log 15.1 LoginManager::SendLoginServerAck, token:" << token.c_str()
                 << " socket:" << it->second;
        m_net_codec.WriteMessage(MessageId::kLoginAck, data.c_str(), data.size(), it->second);
        m_connected_clients.erase(it);
    }
}

void LoginManager::HandleVerifyAck(verify::VerifyAck &ack)
{
    bool is_ok = ack.is_ok();
    std::string token = ack.token();
    
    qDebug() << "trace log 14 LoginManager::HandleVerifyAck, is_ok:" << is_ok << " token:" << token.c_str();
    
    this->SendLoginAck(is_ok, token);
}

void LoginManager::SendVerifyReq(QTcpSocket *socket)
{
    qDebug() << "trace log 11 LoginManager::SendVerifyReq, token:" << m_token.c_str() << " m_tcp_client:" << m_tcp_client << " socket:" << socket;
    verify::VerifyReq verifyReq;
    verifyReq.set_token(m_token);
    std::string data;
    verifyReq.SerializeToString(&data);
    
    m_net_codec.WriteMessage(MessageId::kVerifyReq, data.c_str(), data.size(), socket);
}

void LoginManager::HandleQuitReq(QTcpSocket *socket, verify::QuitReq &req)
{
    qDebug() << "trace log 18 LoginManager::HandleQuitReq";
    
    this->SendQuitAck(socket);
}

void LoginManager::SendQuitAck(QTcpSocket *socket)
{
    qDebug() << "trace log 19 LoginManager::SendQuitAck";
    
    // 理论上，这里应该有登录的现场需要删除，现在实现简单，没有任何现场数据，直接回复消息
    verify::QuitAck quitAck;
    quitAck.set_is_ok(true);
    quitAck.set_err_info("");
    std::string data;
    quitAck.SerializeToString(&data);
    m_net_codec.WriteMessage(MessageId::kQuitAck, data.c_str(), data.size(), socket);
} 
