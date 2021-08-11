//
// Created by cc on 8/9/21.
//

#ifndef CC_WEBSERVER_MUDUO_EVENTLOOPTHREADPOOL_H
#define CC_WEBSERVER_MUDUO_EVENTLOOPTHREADPOOL_H

#include "noncopyable.h"
#include <boost/ptr_container/ptr_vector.hpp>
class EventLoop;
class EventLoopThread;



class EventLoopThreadPool : noncopyable
{
public:
    EventLoopThreadPool(EventLoop* base);
    ~EventLoopThreadPool();
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start();
    EventLoop* getNextLoop();

private:
    EventLoop* baseLoop_;
    bool started_;
    int numThreads_;
    int next_;  // always in loop thread
    boost::ptr_vector<EventLoopThread> threads_;
    std::vector<EventLoop*> loops_;

};



#endif //CC_WEBSERVER_MUDUO_EVENTLOOPTHREADPOOL_H
