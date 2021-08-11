//
// Created by cc on 8/5/21.
//

#ifndef CC_WEBSERVER_MUDUO_EVENTLOOPTHREAD_H
#define CC_WEBSERVER_MUDUO_EVENTLOOPTHREAD_H

#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"


#include <noncopyable.h>



class EventLoop;

class EventLoopThread : noncopyable
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();

    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
};




#endif //CC_WEBSERVER_MUDUO_EVENTLOOPTHREAD_H
