#include "verifymanager.h"
#include "../Common/verify.pb.h"
#include "../Common/comDefinition.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QtEndian>
#include <map>
#include <string>
#include <functional>

std::map<std::string, std::string> g_name_pwd_map;
std::map<std::string, std::string> g_user_token_map;

VerifyManager::VerifyManager(QObject *parent)
    : QObject{parent}, m_net_codec(parent)
{
    m_tcp_server = new QTcpServer(this);
    
    connect(m_tcp_server, &QTcpServer::newConnection, this, &VerifyManager::NewConnection);
    
    if(! m_tcp_server->listen(QHostAddress::SpecialAddress::LocalHost, 5060))
    {
        qDebug() << "Verify Server 启动失败";
    }
    else
    {
        qDebug() << "Verify Server 启动成功";
    }
}

void VerifyManager::Initialize()
{
    m_net_codec.SetMessageCallback(std::bind(&VerifyManager::HandleMessage, this
                                             , std::placeholders::_1, std::placeholders::_2
                                             , std::placeholders::_3, std::placeholders::_4));
}

void VerifyManager::NewConnection()
{
    QTcpSocket *socket = m_tcp_server->nextPendingConnection();
    
    connect(socket, &QTcpSocket::disconnected, this, &VerifyManager::Disconnected);
    
    connect(socket, &QTcpSocket::readyRead, &m_net_codec, &NetworkCodec::ReadyRead);
    
    m_net_codec.RegisterSocket(socket);
    
    qDebug() << "收到新的客户连接 IP:" << socket->peerAddress().toString() << "Port" << socket->peerPort();
}

void VerifyManager::Disconnected()
{
    QTcpSocket *socket = (QTcpSocket*)sender();
    
    qDebug() << "连接断开";
    
    m_net_codec.UnRegisterSocket(socket);
    
    socket->deleteLater();
}

void VerifyManager::HandleMessage(QTcpSocket *socket, unsigned int message_id, const char *data, int len)
{
    qDebug() << "进入到HandleMessage, messsage_id:" << message_id << " len:" << len;
    
    switch(message_id)
    {
        case MessageId::kRegisterReq:
        {
            verify::RegisterReq registerReq;
            registerReq.ParseFromString(data);
            HandleRegisterReq(socket, registerReq);
            break;
        }
        case MessageId::kGetTokenReq:
        {
            verify::GetTokenReq getTokenReq;
            getTokenReq.ParseFromString(data);
            HandleGetTokenReq(socket, getTokenReq);
            break;
        }
        case MessageId::kVerifyReq:
        {
            verify::VerifyReq verifyReq;
            verifyReq.ParseFromString(data);
            HandleVerifyReq(socket, verifyReq);
            break;
        }
        default:
            break;
    }
    
}

void VerifyManager::HandleRegisterReq(QTcpSocket *socket, verify::RegisterReq &req)
{
    qDebug() << "trace log 2 VerifyManager::HandleRegisterReq";
    
    std::string name = req.name();
    std::string passpwd = req.passwd();
    
    qDebug() << "服务器收到消息RegisterReq, name:" << name.c_str() << " passpwd:" << passpwd.c_str();
    auto it = g_name_pwd_map.find(name);
    bool is_ok = true;
    std::string error_info;
    if (it == g_name_pwd_map.end())
    {
        g_name_pwd_map[name] = passpwd;
        qDebug() << "注册成功";
     }
    else
    {
        // 重名
        is_ok = false;
        error_info = "重名";
        qDebug() << "注册失败，重名";
    }
    SendRegisterAck(socket, is_ok, error_info);
}

void VerifyManager::SendRegisterAck(QTcpSocket *socket, bool is_ok, const std::string &error_info)
{
    qDebug() << "trace log 3 VerifyManager::SendRegisterAck";
    verify::RegisterAck regAck;
    regAck.set_is_ok(is_ok);
    regAck.set_error_info(error_info);
    std::string data;
    regAck.SerializeToString(&data);

    m_net_codec.WriteMessage(MessageId::kRegisterAck, data.c_str(), data.size(), socket);
}

void VerifyManager::HandleGetTokenReq(QTcpSocket *socket, verify::GetTokenReq &req)
{
    std::string name = req.name();
    std::string passpwd = req.passwd();
    
    qDebug() << "trace log 6 VerifyManager::HandleLoginReq, name:" << name.c_str() << " passpwd:" << passpwd.c_str();

    auto it = g_name_pwd_map.find(name);
    bool is_ok = false;
    if (it != g_name_pwd_map.end()  && it->second == passpwd)
    {
        is_ok = true;
    }
    
    this->SendGetTokenAck(socket, is_ok, name);
}

void VerifyManager::SendGetTokenAck(QTcpSocket *socket, bool is_ok, const std::string &name)
{
    std::string token;
    verify::GetTokenAck getTokenAck;
    if(is_ok)
    {
        token = "111cccaaa"; // todo: 增加一个生成token的算法
        g_user_token_map[token] = name;
    }
    
    qDebug() << "trace log 7 VerifyManager::SendLoginAck, is_ok:" << is_ok << " token:" << token.c_str();
    
    getTokenAck.set_is_ok(is_ok);
    getTokenAck.set_token(token);
    std::string data;
    getTokenAck.SerializeToString(&data);

    m_net_codec.WriteMessage(MessageId::kGetToenAck, data.c_str(), data.size(), socket);
}

void VerifyManager::HandleVerifyReq(QTcpSocket *socket, verify::VerifyReq &req)
{
    std::string token = req.token();
    
    qDebug() << "trace log 12 VerifyManager::HandleVerifyReq,token:" << token.c_str();
    
    bool is_ok = false;
    std::string ret_name;
    auto it = g_user_token_map.find(token);
    if (it != g_user_token_map.end())
    {
        is_ok = true;
        ret_name = it->second;
        qDebug() << "VerifyServer找到了token验证";
        g_user_token_map.erase(it); // 理论上token需要保存一段时间然后过期，类似于手游的登录流程，现在简单点，直接删除
    }
    
    this->SendVerifyAck(socket, is_ok, token);
}

void VerifyManager::SendVerifyAck(QTcpSocket *socket, bool is_ok, const std::string &token)
{
    qDebug() << "trace log 13 VerifyManager::SendVerifyAck, is_ok:" << is_ok << " token:" << token.c_str();
    
    verify::VerifyAck verifyAck;
    verifyAck.set_is_ok(is_ok);
    verifyAck.set_token(token);
    std::string data;
    verifyAck.SerializeToString(&data);

    m_net_codec.WriteMessage(MessageId::kVerifyAck, data.c_str(), data.size(), socket);
}


