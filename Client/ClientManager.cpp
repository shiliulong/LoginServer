#include "ClientManager.h"
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QTcpSocket>
#include <QtEndian>
//#include "../Common/verify.pb.h"
#include "../Common/comDefinition.h"
#include <QQmlContext>

ClientManager::ClientManager(QObject *parent) : QObject(parent), m_net_codec(parent)
{
    m_tcp_client = new QTcpSocket(this);
    
    connect(m_tcp_client, &QTcpSocket::connected, this, &ClientManager::connected);       //连接上时被动触发
    connect(m_tcp_client, &QTcpSocket::disconnected, this, &ClientManager::disconnected);
}


void ClientManager::Initialize()
{
    m_net_codec.SetMessageCallback(std::bind(&ClientManager::HandleMessage, this
                                            , std::placeholders::_1, std::placeholders::_2
                                            , std::placeholders::_3, std::placeholders::_4));   //疑惑1 延时传值
}

void ClientManager::setQmlEngine(QQmlApplicationEngine *engine)
{
    m_qml_engine = engine;
}

void ClientManager::handleRegister(const QString &username, const QString &password)
{
    qDebug() << "handlerRegister, name:" << username << " password:" << password;
    
    m_user_name = username;
    m_password = password;
    m_state = LoginState_Register;
    if (m_tcp_client == nullptr)
    {
        qDebug() << "LoginState_Register m_tcp_client is nullptr";
    }
    else
    {
        qDebug() << "LoginState_Register m_tcp_client is not nullptr";
    }
    m_tcp_client->connectToHost("localhost", 5060);
}

void ClientManager::handleLogin(const QString &username, const QString &password)
{
    // 在这里处理登录逻辑
    qDebug() << "handleLogin" << " Username:" << username << " Password:" << password;
    
    m_user_name = username;
    m_password = password;
    
    m_state = LoginState_GetToken;
    if (m_tcp_client == nullptr)
    {
        qDebug() << "LoginState_Login m_tcp_client is nullptr tcp_client:" << m_tcp_client;
    }
    else
    {
        qDebug() << "LoginState_Login m_tcp_client is not nullptr:" << m_tcp_client;
    }
    qDebug() << "LoginState_Login m_tcp_client 2222222:";
    m_tcp_client->connectToHost("localhost", 5060);
    
}

void ClientManager::handleQuit()
{
    this->SendQuitReq(m_tcp_client);
}

void ClientManager::connected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    
    if (socket == nullptr)
    {
        qDebug() << "ClientManager::connnected socket is nullptr, socket:" << socket;
    }
    else
    {
        qDebug() << "ClientManager::connnected socket is not nullptr, socket:" << socket;
    }
    
    if (socket != nullptr)
    {
        m_net_codec.RegisterSocket(socket);
        connect(socket, &QTcpSocket::readyRead, &m_net_codec, &NetworkCodec::ReadyRead);
    }

    switch(m_state)
    {
    case LoginState_Register:
    {
        SendRegisterReq(m_tcp_client);
        // 使命完成   
        m_state = LoginState_None;
        break;
    }
    case LoginState_GetToken:
    {
        SendGetTokenReq(m_tcp_client);
        break;
    }
    case LoginState_Login:
    {
        SendLoginReq(m_tcp_client);
        // 使命完成   
        m_state = LoginState_None;
        break;
    }
    default:
        break;
    }
}

void ClientManager::disconnected() // 回调函数  异步
{
    QTcpSocket *socket = (QTcpSocket*)sender();
    
    qDebug() << "连接断开, socket:" << socket;
    
    m_net_codec.UnRegisterSocket(socket);
    
    
    if (m_state == LoginState_GetToken)
    {
        // 使命完成   
        m_state = LoginState_Login;
        m_tcp_client->connectToHost("localhost", 6060);
    }
}



void ClientManager::HandleMessage(QTcpSocket* socket, unsigned int message_id, const char *data, int len)
{
    qDebug() << "进入到ClientManager::HandleMessage, messsage_id:" << message_id << " len:" << len;
    switch(message_id)
    {
    case MessageId::kRegisterAck:
    {
        verify::RegisterAck registerAck;
        registerAck.ParseFromString(data);
        HandleRegisterAck(registerAck);
        
        socket->close(); // 短连接，早点断开
        break;
    }
    case MessageId::kGetToenAck:
    {
        verify::GetTokenAck getTokenAck;
        getTokenAck.ParseFromString(data);
        HandleGetTokenAck(socket, getTokenAck);
        break;
    }
    case MessageId::kLoginAck:
    {
        verify::LoginAck loginAck;
        loginAck.ParseFromString(data);
        HandleLoginAck(loginAck);  // 这里是长连接，不断开，后面如果退出，后面可以退出
        
        //socket->close();
        this->m_curLoginSocket = socket; // 储存当前连接的服务器
        break;
    }
    case MessageId::kQuitAck:
    {
        verify::QuitAck quitAck;
        quitAck.ParseFromString(data);
        HandleQuitAck(quitAck);
        break;
    }
    default:
        break;
    }
}

void ClientManager::SendRegisterReq(QTcpSocket* socket)
{
    qDebug() << "trace log 1 ClientManager::SendRegisterReq";
    verify::RegisterReq regReq;
    regReq.set_name(m_user_name.toStdString().c_str());
    regReq.set_passwd(m_password.toStdString().c_str());
    std::string data;
    regReq.SerializeToString(&data);


    m_net_codec.WriteMessage(MessageId::kRegisterReq, data.c_str(), data.size(), socket);
}

void ClientManager::HandleRegisterAck(verify::RegisterAck ack)
{
    bool is_ok = ack.is_ok();
    std::string error_info = ack.error_info();
    
    qDebug() << "trace log 4 ClientManager::HandleRegisterAck, is_ok:" << is_ok << " error_info:" << error_info.c_str();
    
    emit registerCallback(is_ok);
}


void ClientManager::SendGetTokenReq(QTcpSocket* socket)
{
    qDebug() << "trace log 5 ClientManager::SendLoginReq";
    verify::GetTokenReq getTokenReq;
    getTokenReq.set_name(m_user_name.toStdString().c_str());
    getTokenReq.set_passwd(m_password.toStdString().c_str());
    std::string data;
    getTokenReq.SerializeToString(&data);


    m_net_codec.WriteMessage(MessageId::kGetTokenReq, data.c_str(), data.size(), socket);
}

void ClientManager::HandleGetTokenAck(QTcpSocket* socket, verify::GetTokenAck ack)
{
    bool is_ok = ack.is_ok();
    std::string token = ack.token();
    
    qDebug() << "trace log 8 ClientManager::HandleLoginAck, is_ok:" << is_ok << " token:" << token.c_str();
    
    if (is_ok) {
        // 发消息到LandingServer
        m_token = token;
    } else {
        emit loginCallback(is_ok);
    }
    
    socket->close();
}

void ClientManager::SendLoginReq(QTcpSocket* socket)
{
    qDebug() << "trace log 9 ClientManager::SendLandingReq, token:" << m_token.c_str();
    verify::LoginReq loginReq;
    loginReq.set_token(m_token);
    std::string data;
    loginReq.SerializeToString(&data);
    m_net_codec.WriteMessage(MessageId::kLoginReq, data.c_str(), data.size(), socket);
}

void ClientManager::HandleLoginAck(verify::LoginAck ack)
{
    bool is_ok = ack.is_ok();
    
    qDebug() << "trace log 16 Received LandingAck, is_ok:" << is_ok;
    
    emit loginCallback(is_ok);
}


void ClientManager::SendQuitReq(QTcpSocket* socket)
{
    qDebug() << "trace log 17 ClientManager::SendQuitReq";
    verify::QuitReq quitReq;
    std::string data;
    quitReq.SerializeToString(&data);
    m_net_codec.WriteMessage(MessageId::kQuitReq, data.c_str(), data.size(), socket);
}

void ClientManager::HandleQuitAck(verify::QuitAck ack)
{
    qDebug() << "trace log 20 ClientManager::SendQuitReq";
    emit quitCallback(true);
    m_tcp_client->close();
}

void ClientManager::logOut()
{
    m_curLoginSocket->close();
}
