//
// Created by cc on 8/4/21.
//

#include "Channel.h"
#include "EventLoop.h"

#include "poll.h"
#include "iostream"


const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd)
  : loop_(loop),
    fd_(fd),
    events_(0),
    revents_(0),
    index_(-1)
{

}

Channel::~Channel() {
    assert(!eventHandleing);
}

void Channel::update() {
    loop_->updateChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    eventHandleing = true;
    if (revents_ & POLLNVAL) {
       std::cout << "Channel::handle_event() POLLNVAL" << std::endl;
    }
    if((revents_ & POLLHUP) && !(revents_ & POLLIN))
        if(closeCallback_) closeCallback_();
    if (revents_ & (POLLERR | POLLNVAL)) {
        if (errorCallback_) errorCallback_();
    }
    //POLLRDHUP接收到对方关闭连接的请求
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if (readCallback_) readCallback_(receiveTime);
    }
    if (revents_ & POLLOUT) {
        if (writeCallback_) writeCallback_();
    }
    eventHandleing = false;
}