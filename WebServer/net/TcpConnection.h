//
// Created by cc on 8/6/21.
//

#ifndef CC_WEBSERVER_MUDUO_TCPCONNECTION_H
#define CC_WEBSERVER_MUDUO_TCPCONNECTION_H

#include "InetAddress.h"
#include "noncopyable.h"
#include <memory>
#include "Callbacks.h"
#include "Buffer.h"

class Timestamp;
class Channel;
class Socket;
class EventLoop;

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection> {
public:
    //使用连接的socket创建，用户不会自己create this object
    TcpConnection(EventLoop* loop,
                  const std::string& name,
                  int sockfd,
                  const InetAddress& localAddr,
                  const InetAddress& peerAddr);
    ~TcpConnection();
    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() { return localAddr_; }
    const InetAddress& peerAddress() { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }

    //两个都可以跨线程使用（处理数据可能是在另外的线程吗，那发送数据在另外的线程也可以吧，这样会损失一点性能，但线程安全性很容易验证） thread safe
    void send(const std::string& message);

    void shutdown();
    void setTcpNoDelay(bool on);
    void setKeepAlive(bool on);

    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    /// Internal use only.
    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }


    // called when TcpServer accepts a new connection
    void connectEstablished();   // should be called only once

    void connectDestroyed();   // should be called only once



private:
    enum StateE { kConnecting, kConnected, kDisconnecting, kDisconnected, };
    void setState(StateE s)  { state_ = s; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    //为send和shut的InLoop函数
    void sendInLoop(const std::string& message);
    void shutdownInLoop();

    EventLoop* loop_;
    std::string name_;
    StateE state_;
    //TOSEE 啥叫we don‘t expose those classes to client, channel和socket会随着tcpConnection析构
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    InetAddress localAddr_;
    InetAddress peerAddr_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;

};

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

#endif //CC_WEBSERVER_MUDUO_TCPCONNECTION_H
