//
// Created by cc on 8/6/21.
//

#ifndef CC_WEBSERVER_MUDUO_ACCEPTOR_H
#define CC_WEBSERVER_MUDUO_ACCEPTOR_H
#include "Acceptor.h"
#include "noncopyable.h"
#include <functional>
#include "Socket.h"
#include "Channel.h"


class EventLoop;
class InetAddress;

class Acceptor : noncopyable {
public:
    typedef std::function<void(int sockfd, const InetAddress &)> NewConnectionCallback;

    //InetAddress使用引用
    Acceptor(EventLoop *loop, const InetAddress &listenAddr);

    void setNewConnectionCallback(const NewConnectionCallback &cb) { newConnectionCallback_ = cb; }
    bool listenning() const { return listenning_; }
    void listen();


private:
    void handleRead();

    EventLoop *loop_;

    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;


};


#endif //CC_WEBSERVER_MUDUO_ACCEPTOR_H
