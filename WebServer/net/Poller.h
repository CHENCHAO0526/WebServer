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

struct pollfd;

class Channel;

class Poller : noncopyable {
public:
    typedef std::vector<Channel*> ChannelList;
    Poller(EventLoop* loop);
    ~Poller();

//poll event时间，在loop中肯定会被调用
    Timestamp poll(int timeoutMs, ChannelList* activeChannels);

    // 改变感兴趣的IO时间，loop thread中必须调用
    void updateChannel(Channel* channel);

    //removeChannel, 当它毁灭时，必须在loop thread
    void removeChannel(Channel* channel);
    //
    void assertInLoopThread() { ownerLoop_ -> assertInLoopThread(); }



private:
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    typedef std::vector<struct pollfd> PollFdList;
    //fd到channel的映射
    typedef std::map<int, Channel*> ChannelMap;

    EventLoop* ownerLoop_;
    PollFdList pollfds_;
    ChannelMap channels_;

};


#endif //CC_WEBSERVER_MUDUO_POLLER_H
