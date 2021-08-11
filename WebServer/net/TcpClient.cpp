//
// Created by cc on 8/11/21.
//

#include "TcpClient.h"
#include "Logging.h"
#include "Connector.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include "TcpConnection.h"
#include <iostream>


#include <stdio.h>  // snprintf

void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
{
    loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(const ConnectorPtr& connector)
{
    //connector->
}

TcpClient::TcpClient(EventLoop* loop,
                     const InetAddress& serverAddr,
                     const string& nameArg)
        : loop_(loop),
          connector_(new Connector(loop, serverAddr)),
          name_(nameArg),
          connectionCallback_(defaultConnectionCallback),
          messageCallback_(defaultMessageCallback),
          retry_(false),
          connect_(true),
          nextConnId_(1)
{
    connector_->setNewConnectionCallback(
            std::bind(&TcpClient::newConnection, this, _1));
    // FIXME setConnectFailedCallback
    std::cout<<"INFO " << "TcpClient::TcpClient[" << name_
             << "] - connector " << get_pointer(connector_)<<std::endl;
}


TcpClient::~TcpClient() {
    std::cout<<"INFO " << "TcpClient::~TcpClient[" << name_
             << "] - connector " << get_pointer(connector_) <<std::endl;
    TcpConnectionPtr conn;

}

void TcpClient::connect()
{
    // FIXME: check state
    std::cout<<"INFO "  << "TcpClient::connect[" << name_ << "] - connecting to "
             << connector_->serverAddress().toHostPort() <<std::endl;
    connect_ = true;
    connector_->start();
}


void TcpClient::disconnect()
{
    connect_ = false;

    {
        MutexLockGuard lock(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(sockets::getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toHostPort().c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;

    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
    TcpConnectionPtr conn(new TcpConnection(loop_,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
            std::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe
    {
        MutexLockGuard lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        MutexLockGuard lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    if (retry_ && connect_)
    {
        std::cout<<"INFO " << "TcpClient::connect[" << name_ << "] - Reconnecting to "
                 << connector_->serverAddress().toHostPort() << std::endl;
        connector_->restart();
    }
}





