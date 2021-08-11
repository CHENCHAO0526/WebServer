//
// Created by cc on 8/4/21.
//

#ifndef CC_WEBSERVER_MUDUO_POLLER_H
#define CC_WEBSERVER_MUDUO_POLLER_H

#include "noncopyable.h"
#include <vector>
#include <map>
#include "Timestamp.h"
#include "EventLoop.h"


class Channel;

class Poller : noncopyable {
public:
    typedef std::vector<Channel*> ChannelList;
    Poller(EventLoop* loop);
    virtual ~Poller();

//poll event时间，在loop中肯定会被调用
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

    // 改变感兴趣的IO时间，loop thread中必须调用
    virtual void updateChannel(Channel* channel) = 0;

    //removeChannel, 当它毁灭时，必须在loop thread
    virtual void removeChannel(Channel* channel) = 0;

    virtual bool hasChannel(Channel* channel) const;

    static Poller* newDefaultPoller(EventLoop* loop);
    //
    virtual void assertInLoopThread()  const { ownerLoop_ -> assertInLoopThread(); }


protected:
    typedef std::map<int, Channel*> ChannelMap;
    ChannelMap channels_;

private:
    EventLoop* ownerLoop_;

};


#endif //CC_WEBSERVER_MUDUO_POLLER_H
