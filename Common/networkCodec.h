#ifndef NetworkCodec_H
#define NetworkCodec_H

#include <QObject>
#include <QByteArray>
#include <map>

class QTcpSocket;
typedef std::function<void (QTcpSocket* socket, unsigned int message_id, const char* data, int len)> MessageCallback;

struct TcpSocketInfo
{
    QByteArray byte_array_;
};

class NetworkCodec : public QObject
{
    Q_OBJECT
public:
    NetworkCodec(QObject *parent);
    
    void RegisterSocket(QTcpSocket* socket);
    void UnRegisterSocket(QTcpSocket* socket);
    void SetMessageCallback(MessageCallback message_callback);
    
    void ReadyRead(void);
    void WriteMessage(unsigned int message_id, const char* data, int len, QTcpSocket* socket = nullptr);
    
private:
    std::map<QTcpSocket*, TcpSocketInfo> tcp_socket_info_;
    MessageCallback message_callback_;
};

#endif // NetworkCodec_H
