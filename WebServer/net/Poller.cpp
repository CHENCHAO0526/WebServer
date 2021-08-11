//
// Created by cc on 8/4/21.
//

#include "Poller.h"
#include "Channel.h"

//#include "logging/Logging.h"



//poller对应于一个loop，生命周期也一致
Poller::Poller(EventLoop* loop)
        : ownerLoop_(loop)
{
}

Poller::~Poller()
{
}

bool Poller::hasChannel(Channel* channel) const
{
    assertInLoopThread();
    ChannelMap::const_iterator it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}






