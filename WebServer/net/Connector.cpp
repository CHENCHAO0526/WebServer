//
// Created by cc on 8/10/21.
//

#include "Connector.h"
#include "Channel.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include <iostream>
#include <errno.h>
#include "Logging.h"


const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
        : loop_(loop),
          serverAddr_(serverAddr),
          connect_(false),
          state_(kDisconnected),
          retryDelayMs_(kInitRetryDelayMs)
{
    std::cout << "DEBUG Connector " << "ctor[" << this << "]" <<std::endl;
}

Connector::~Connector()
{
    std::cout << "DEBUG Connector " << "dtor[" << this << "]" <<std::endl;
    loop_->cancel(timerId_);
    assert(!channel_);
}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this)); // FIXME: unsafe
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_)
    {
        connect();
    }
    else
    {
        std::cout << "DEBUG Connector " << "do not connect";
    }
}


//connect对不同错误的处理方法
void Connector::connect()
{
    int sockfd = sockets::createNonblockingOrDie();
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddrInet());
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sockfd);          //进一步判断
            break;

        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sockfd);
            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            std::cout <<"SYSERR " << "connect error in Connector::startInLoop " << savedErrno;
            sockets::close(sockfd);
            break;

        default:
            std::cout <<"SYSERR "  << "Unexpected error in Connector::startInLoop " << savedErrno;
            sockets::close(sockfd);
            // connectErrorCallback_();
            break;
    }
}

void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}



void Connector::stop()
{
    connect_ = false;
    //处理定时器超时后COnnector的生命周期问题
    loop_->cancel(timerId_);
}


void Connector::connecting(int sockfd) {
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(
            std::bind(&Connector::handleWrite, this)); // FIXME: unsafe
    channel_->setErrorCallback(
            std::bind(&Connector::handleError, this)); // FIXME: unsafe

    channel_->enableWriting();
}

int Connector::removeAndResetChannel() {
    channel_->disableAll();
    //之前使用get_pointer,这里对unique-ptr使用了get,使用uniqueptr的原因是为了管理周期，并且不暴露
    loop_->removeChannel(channel_.get());
    int sockfd = channel_->fd();
    // 不能在这resetchannel——，因为处于Chanel::handleEvent, Can't reset channel_ here, because we are inside Channel::handleEvent
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this)); // FIXME: unsafe this可能已经被析构
    return sockfd;
}

void Connector::resetChannel() {
    channel_.reset();
}

void Connector::handleWrite() {
    std::cout << "Connector::handleWrite: " << state_ << std::endl;
    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        //及时可读还是需要检查err
        int err = sockets::getSocketError(sockfd);
        if (err)
        {
            std::cout << "WARN " << "Connector::handleWrite - SO_ERROR = "
                     << err << " " << strerror_tl(err);
            retry(sockfd);
        }
        //处理自连接
        else if (sockets::isSelfConnect(sockfd))
        {
            std::cout << "WARN " << "Connector::handleWrite - Self connect";
            retry(sockfd);
        }
        else
        {
            setState(kConnected);
            if (connect_)
            {
                newConnectionCallback_(sockfd);
            }
            else
            {
                sockets::close(sockfd);
            }
        }
    }
    else
    {
        // what happened?
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError()
{
    std::cout << "ERROR " << "Connector::handleError" << std::endl;
    assert(state_ == kConnecting);

    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);
    std::cout << "TRACE " << "SO_ERROR = " << err << " " << strerror_tl(err);
    retry(sockfd);
}

void Connector::retry(int sockfd)
{
    sockets::close(sockfd);
    setState(kDisconnected);
    if (connect_)
    {
        std::cout << "INFO "  << "Connector::retry - Retry connecting to "
                 << serverAddr_.toHostPort() << " in "
                 << retryDelayMs_ << " milliseconds. " << std::endl;
        timerId_ = loop_->runAfter(retryDelayMs_/1000.0,  // FIXME: unsafe  改成shared_from_this?  这里做法是在析构函数
                                                                 // 里注销定时器,确保Connector
                                   std::bind(&Connector::startInLoop, this));
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    }
    else
    {
        std::cout << "INFO "   << "do not connect" << std::endl;
    }
}