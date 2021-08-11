//
// Created by cc on 8/3/21.
//

#ifndef WEBSERVER_MUDUO_CC_EVENTLOOP_H
#define WEBSERVER_MUDUO_CC_EVENTLOOP_H

#include "Thread.h"
#include <vector>
#include "TimerId.h"
#include <memory>
#include "Timestamp.h"
#include "Callbacks.h"
#include <functional>
#include "Mutex.h"

class Channel;
class Poller;
class TimerQueue;

class EventLoop  {
public:
    typedef std::function<void()> Functor;


    EventLoop();

    ~EventLoop();

    void loop();

    void quit();
    void wakeup();

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

    void assertInLoopThread() {
        if(!isInLoopThread())
            abortNotInLoopThread();
    }
     bool isInLoopThread() const {
        return threadId_ == CurrentThread::tid();
    }
    /// Runs callback immediately in the loop thread.
    /// It wakes up the loop, and run the cb.
    /// If in the same loop thread, cb is run within the function.
    /// Safe to call from other threads.
    void runInLoop(const Functor& cb);
    /// Queues callback in the loop thread.
    /// Runs after finish pooling.
    /// Safe to call from other threads.
    void queueInLoop(const Functor& cb);
    //调用timequeue，for timer
    TimerId runAt(const Timestamp& time, const TimerCallback& cb);
    ///
    /// Runs callback after @c delay seconds.
    ///
    TimerId runAfter(double delay, const TimerCallback& cb);
    ///
    /// Runs callback every @c interval seconds.
    ///
    TimerId runEvery(double interval, const TimerCallback& cb);

    void cancel(TimerId timerId);


private:
    void handleRead();
    void doPendingFunctors();
    typedef std::vector<Channel*> ChannelList;
    void abortNotInLoopThread();
    ChannelList activeChannels_;
    bool looping_;
    bool quit_;
    bool callingPendingFunctors_; /* atomic */
    const pid_t threadId_;
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;
    int wakeupFd_;
    // unlike in TimerQueue, which is an internal class,
    // we don't expose Channel to client.
    //TOSEE 没太搞懂这里
    //这里用weaktr，channel里直接用的internal class
    std::unique_ptr<Channel> wakeupChannel_;
    MutexLock mutex_;
    std::vector<Functor> pendingFunctors_; // @GuardedBy mutex_

};

#endif //WEBSERVER_MUDUO_CC_EVENTLOOP_H
