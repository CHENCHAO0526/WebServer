//
// Created by cc on 8/6/21.
//

#include "TcpConnection.h"

#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "SocketsOps.h"

#include <errno.h>
#include <stdio.h>
#include <iostream>
#include <functional>
//class Timestamp;

void defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    std::cout << "TRACE " << conn->localAddress().toHostPort() << " -> "
              << conn->peerAddress().toHostPort()<< " is "
              << (conn->connected() ? "UP" : "DOWN") << std::endl;
    // do not call conn->forceClose(), because some users want to register message callback only.
}

void defaultMessageCallback(const TcpConnectionPtr&,
                                        Buffer* buf,
                                        Timestamp)
{
    buf->retrieveAll();
}


TcpConnection::TcpConnection(EventLoop* loop,
                             const std::string& nameArg,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
     //check not NULL
        : loop_(loop),
          name_(nameArg),
          state_(kConnecting),
          socket_(new Socket(sockfd)),
          channel_(new Channel(loop, sockfd)),
          localAddr_(localAddr),
          peerAddr_(peerAddr),
          highWaterMark_(64*1024*1024)
{
    std::cout << "Debug" << "TcpConnection::ctor[" <<  name_ << "] at " << this
              << " fd=" << sockfd << std::endl;
    //高层设置底层的回调函数
    channel_->setReadCallback(
            std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
            std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
            std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
            std::bind(&TcpConnection::handleError, this));
}

TcpConnection::~TcpConnection()
{
    std::cout << "Debug" << "TcpConnection::dtor[" <<  name_ << "] at " << this
              << " fd=" << channel_->fd() << std::endl;
    assert(state_ == kDisconnected);
}

void TcpConnection::send(const std::string &message) {
    if(state_ == kConnected) {
        if(loop_->isInLoopThread()) {
            sendInLoop(message);
        } else {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, message));
        }
    }
}

//容易验证线程安全性
//level 触发写的逻辑好复杂
void TcpConnection::sendInLoop(const std::string &message) {
    //需要访问共享资源，显然只能单线程安全
    loop_->assertInLoopThread();
    ssize_t nwrote;
    size_t remaining = message.size(), len = remaining;
    bool faultError = false;
    if (state_ == kDisconnected)
    {
        std::cout<< "WARN" << "disconnected, give up writing" << std::endl;
        return;
    }
    //先尝试直接发送数据，如果现在不是writing状态并且buffer中没有缓存 可以直接发送，    是writing状态代表什么下面运行过一次，那么buffer里就有数据，感觉有点重复了，是不是只用判断buffer就好了，不太确定了
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), message.data(), message.size());   //只调用一次，因为调用第二次很可能EAGIN
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (implicit_cast<size_t>(nwrote) < message.size()) {
                std::cout << " TRAVE " << "I am going to write more data" << std::endl;
            } else if(writeCompleteCallback_) {
                //为什么要queueInLoop，而不是马上就执行呢
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                std::cout << "SYSERR " <<"TcpConnection::sendInLoop" << std::endl;
            }
            if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
            {
                faultError = true;
            }
        }
    }
    //没发送玩的话需要打开channel的writing，存在buffer里下次发
    assert(nwrote >= 0);
    if (!faultError && implicit_cast<size_t>(nwrote) < message.size()) {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(message.data()+nwrote, message.size()-nwrote);
        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }

}

void TcpConnection::shutdown() {
    if(state_ == kConnected) {
        setState(kDisconnecting);
        //FIXME sharedFromThis ?
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if(!channel_->isWriting()) {  //当数据发送完后才会shutdown socket
        //使用shutdown(sockfd, SHUT_WR), 关闭写，注意与close的不同，close（fd), fd在其他进程仍然有效，shutdown后其他进程也不能写了，并且shutdown可选shutdown or write
        // 优雅的shutdown，等到read返回0再关闭连接
        socket_->shutdownWrite();
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

void TcpConnection::setKeepAlive(bool on)
{
    socket_->setKeepAlive(on);
}

void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::handleRead(Timestamp reveiveTime)
{
    int saveError = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveError);
    if(n >0)
        messageCallback_(shared_from_this(), &inputBuffer_, reveiveTime);
    else if(n==0) {
        handleClose();
    } else {
        errno = saveError;
        std :: cout << "error " << std::endl;
        handleError();
    }
}

void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if(channel_->isWriting()) {
        ssize_t n = ::write(channel_->fd(),  //只调用一次，因为调用第二次很可能EAGIN
                            outputBuffer_.peek(),
                            outputBuffer_.readableBytes());
        if(n > 0) {
            outputBuffer_.retrieve(n);
            //如果数据写完了
            if(outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();    //数据发送完了关闭wriing，需要时再打开
                if (writeCompleteCallback_) {
                    loop_->queueInLoop(
                            std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting) {     //当数据发送完后判断是否shutdown过进入disconnecting状态，是的话才会shutdown socket
                    shutdownInLoop();
                }
            } else {
                std::cout << "TRACE " << "I am going to write more data" << std::endl;
            }
        } else {
            std::cout <<" SYSERR " <<"TcpConnection::handleWrite" << std::endl;
        }


    } else {   //已经关闭了，比如handleclose、
        std::cout << "TRACE "<<"Connection is down, no more writing" << std::endl;
    }
}

void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    std::cout << "TcpConnection::handleClose state = " << state_ << std::endl;
    assert(state_ == kConnected || state_ == kDisconnecting);
    //fd交给析构函数
    channel_ -> disableAll();
    closeCallback_(shared_from_this());
}

void TcpConnection::handleError() {
    int err = sockets::getSocketError(channel_ -> fd());
    std::cout << "error" << "TcpConnection::handleError [" << name_ << "] - SO_ERROR = " << err << " " << strerror(err) <<std::endl;
}

//TcpConnection析构前最后调用的，有时候不经由handleClose调用调用connectDestroyed
void TcpConnection::connectDestroyed() {
    loop_ -> assertInLoopThread();
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    channel_ -> disableAll();

    connectionCallback_(shared_from_this());
    loop_->removeChannel(channel_.get());
}




