//
// Created by cc on 8/4/21.
//

#ifndef CC_WEBSERVER_MUDUO_CHANNEL_H
#define CC_WEBSERVER_MUDUO_CHANNEL_H

#include "noncopyable.h"
#include "functional"

//前置声明，尽可能减少编译的依赖关系
class EventLoop;
class Timestamp;
/*
 * A selectable IO channel
 * 不拥有文件描述符，文件描述符可以是socket、eventfd、timefd、signalfd
 */

class Channel : public noncopyable {
public:
    typedef std::function<void()> EventCallback;
    typedef std::function<void(Timestamp)> ReadEventCallback;

    Channel(EventLoop* loop, int fd);

    ~Channel();

    //有event::loop调用，根据revents_的值进行不同的用户回调
    void handleEvent(Timestamp receiveTime);

    void setReadCallback(const ReadEventCallback& cb)
    { readCallback_ = cb; }
    void setWriteCallback(const EventCallback& cb)
    { writeCallback_ = cb; }
    void setErrorCallback(const EventCallback& cb)
    { errorCallback_ = cb; }
    void setCloseCallback(const EventCallback& cb)
    { closeCallback_ = cb;}

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    void enableReading() { events_ |= kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    bool isWriting() { return events_ & kWriteEvent; }
    // for Poller
     int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    EventLoop* ownerLoop() { return loop_; }

private:
    void update();
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
    EventLoop* loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_; //poller使用
    bool eventHandleing;

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;
};



#endif //CC_WEBSERVER_MUDUO_CHANNEL_H
