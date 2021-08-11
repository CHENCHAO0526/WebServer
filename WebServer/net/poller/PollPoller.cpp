//
// Created by cc on 8/11/21.
//

#include "PollPoller.h"
#include <poll.h>
#include "../Channel.h"
#include <assert.h>
#include "iostream"

PollPoller::PollPoller(EventLoop* loop)
        : Poller(loop)
{
}

PollPoller::~PollPoller() = default;


//pollpoller的核心，利用函数轮询所有的fd，在使用fillActiveChannel 函数填充activeChannel，这里不会调用handleEvent，因为为了让poller只负责IO复用，不负责事件分发
Timestamp PollPoller::poll(int timeOutMs, ChannelList* activeChannels) {
    int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeOutMs);
    //时刻处理errno
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0) {
        std::cout<< numEvents << " events happended" <<std::endl;
        fillActiveChannels(numEvents, activeChannels);
    } else if (numEvents == 0) {
        std::cout << " nothing happended" <<std::endl;
    } else
    {
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            std::cout << "SYSERR" << "PollPoller::poll()" << std::endl;
        }
    }
    return now;
}

//得到active的channel并设置器revents
void PollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const {
    for(PollFdList::const_iterator pfd = pollfds_.begin(); pfd != pollfds_.end() && numEvents >0; ++pfd) {
        if(pfd->revents > 0) {
            --numEvents;
            ChannelMap::const_iterator ch = channels_.find(pfd->fd);
            assert(ch != channels_.end());
            Channel* channel = ch->second;
            assert(channel->fd() == pfd->fd);
            channel->set_revents(pfd->revents);
            activeChannels->push_back(channel);
        }
    }
}

// 升级channel对应的fd的pollfd的属性
void PollPoller::updateChannel(Channel* channel)
{
    assertInLoopThread();
    std::cout << "TRACE " <<"fd = " << channel->fd() << " events = " << channel->events() <<std::endl;
    if (channel->index() < 0) {
        // a new one, add to pollfds_
        assert(channels_.find(channel->fd()) == channels_.end());
        struct pollfd pfd;
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int idx = static_cast<int>(pollfds_.size())-1;
        channel->set_index(idx);
        channels_[pfd.fd] = channel;
    } else {
        // update existing one
        assert(channels_.find(channel->fd()) != channels_.end());
        assert(channels_[channel->fd()] == channel);
        int idx = channel->index();
        assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
        struct pollfd& pfd = pollfds_[idx];
        assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd()-1);
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        if (channel->isNoneEvent()) {
            // ignore this pollfd 忽略的方式是-fd-1
            pfd.fd = -channel->fd()-1;
        }
    }
}

void PollPoller::removeChannel(Channel* channel) {
    assertInLoopThread();
    std::cout << "TRACE in Poll::removeChannel" << "fd = " << channel->fd() << std::endl;
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    assert(channel->isNoneEvent());
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    const struct pollfd& pfd = pollfds_[idx]; (void)pfd;
    assert(pfd.fd == -channel->fd()-1 && pfd.events == channel->events());
    size_t n = channels_.erase(channel->fd());
    assert(n == 1); (void)n;
    if (implicit_cast<size_t>(idx) == pollfds_.size()-1) {
        pollfds_.pop_back();
    } else {
        int channelAtEnd = pollfds_.back().fd;
        iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);
        if (channelAtEnd < 0) {
            channelAtEnd = -channelAtEnd-1;
        }
        channels_[channelAtEnd]->set_index(idx);
        pollfds_.pop_back();
    }
}
