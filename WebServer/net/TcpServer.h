//
// Created by cc on 8/6/21.
//

#ifndef CC_WEBSERVER_MUDUO_TCPSERVER_H
#define CC_WEBSERVER_MUDUO_TCPSERVER_H


#include "Callbacks.h"
#include "TcpConnection.h"

#include <map>
#include <memory>
#include "noncopyable.h"

class Acceptor;
class EventLoop;
class EventLoopThreadPool;


class TcpServer : noncopyable {
public:

    TcpServer(EventLoop *loop, const InetAddress &listenAddr);

    ~TcpServer();  // force out-line dtor, for scoped_ptr members.

    /// Set the number of threads for handling input.
    ///
    /// Always accepts new connection in loop's thread.
    /// Must be called before @c start
    /// @param numThreads
    /// - 0 means all I/O in loop's thread, no thread will created.
    ///   this is the default value.
    /// - 1 means all I/O in another thread.
    /// - N means a thread pool with N threads, new connections
    ///   are assigned on a round-robin basis.
    void setThreadNum(int numThreads);

    /// Starts the server if it's not listenning.
    ///
    /// It's harmless to call it multiple times.
    /// Thread safe.
    void start();

    /// Set connection callback.
    /// Not thread safe.
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    /// Set message callback.
    /// Not thread safe.
    // 可以用来处理httpdata收到消息时的回调，上层进行设置
    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }
    /// Set write complete callback.
    /// Not thread safe.
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

private:
    /// Not thread safe, but in loop, 传给acceptor
    void newConnection(int sockfd, const InetAddress& peerAddr);
    //增加threadpool后变threadsafe
    void removeConnection(const TcpConnectionPtr& conn);

    /// Not thread safe, but in loop
    void removeConnectionInLoop(const TcpConnectionPtr& conn);
    typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;


    EventLoop* loop_;  // the acceptor loop
    const std::string name_;
    //慢慢明白为什么要使用unique_ptr
    std::unique_ptr<Acceptor> acceptor_; // avoid revealing Acceptor
    std::unique_ptr<EventLoopThreadPool> threadPool_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    bool started_;
    int nextConnId_;  // always in loop thread
    ConnectionMap connections_;
};






#endif //CC_WEBSERVER_MUDUO_TCPSERVER_H
