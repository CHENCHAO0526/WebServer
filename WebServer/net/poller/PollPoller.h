//
// Created by cc on 8/11/21.
//

#ifndef CC_WEBSERVER_MUDUO_POLLPOLLER_H
#define CC_WEBSERVER_MUDUO_POLLPOLLER_H

#include "../Poller.h"

#include <vector>
struct pollfd;


//基于POLL对IO复用的封装
class PollPoller : public Poller
{
public:

    PollPoller(EventLoop* loop);
    ~PollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    void fillActiveChannels(int numEvents,
                            ChannelList* activeChannels) const;

    //poll和epoll使用不同的结构
    typedef std::vector<struct pollfd> PollFdList;
    PollFdList pollfds_;
};



#endif //CC_WEBSERVER_MUDUO_POLLPOLLER_H
