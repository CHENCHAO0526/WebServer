//
// Created by cc on 8/11/21.
//

#ifndef CC_WEBSERVER_MUDUO_EPOLLPOLLER_H
#define CC_WEBSERVER_MUDUO_EPOLLPOLLER_H

#include "../Poller.h"

#include <vector>

struct epoll_event;


class EPollPoller : public Poller {
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;

    //封装epoll ctl
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;
private:
    static const int kInitEventListSize = 16;

    static const char* operationToString(int op);
    //epoll_wait返回后将就绪的文件描述符添加到参数的激活队列中
    void fillActiveChannels(int numEvents,
                            ChannelList* activeChannels) const;
    void update(int operation, Channel* channel);

    typedef std::vector<struct epoll_event> EventList;

    int epollfd_;
    EventList events_;

};


#endif //CC_WEBSERVER_MUDUO_EPOLLPOLLER_H
