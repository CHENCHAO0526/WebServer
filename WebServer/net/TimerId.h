//
// Created by cc on 8/5/21.
//

#ifndef CC_WEBSERVER_MUDUO_TIMERID_H
#define CC_WEBSERVER_MUDUO_TIMERID_H

#include "copyable.h"

class Timer;

//没太搞懂这是在干嘛 An opaque identifier, for canceling Timer.， 区别是这个可以copy
class TimerId : public copyable
{
public:
    TimerId(Timer* timer = NULL, int64_t seq = 0)
            : timer_(timer),
              sequence_(seq)
    {
    }

    // default copy-ctor, dtor and assignment are okay

    friend class TimerQueue;

private:
    Timer* timer_;
    int64_t sequence_;
};






#endif //CC_WEBSERVER_MUDUO_TIMERID_H
