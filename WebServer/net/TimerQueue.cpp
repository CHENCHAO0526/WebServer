//
// Created by cc on 8/5/21.
//

#include "TimerQueue.h"
#include "Timer.h"
#include "TimerId.h"
#include "EventLoop.h"
#include "Channel.h"
#include <sys/timerfd.h>
#include <iostream>
#include <functional>

int createTimerfd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                   TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
    {
        std::cout << "Failed in timerfd_create"<<std::endl;
    }
    return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when) {
    int64_t microseconds = when.microSecondsSinceEpoch()
                           - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(
            microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(
            (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

void resetTimerfd(int timerfd, Timestamp expiration)
{
    // wake up loop by timerfd_settime()
    /*
     *   struct timespec
        {
        time_t tv_sec; //秒
        long tv_nsec; //纳秒
        }
        struct itimerspec
        {
        struct timespec it_interval; //首次超时后，每隔it_interval超时一次
        struct timespec it_value; //首次超时时间
        }

     */
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue, sizeof newValue);
    bzero(&oldValue, sizeof oldValue);
    newValue.it_value = howMuchTimeFromNow(expiration);
    /*
     * flags： 1表示设置的是绝对时间；0表示相对时间。
        newValue： 指定新的超时时间，若newValue.it_value非0则启动定时器，否则关闭定时器。若newValue.it_interval为0则定时器只定时一次，否则之后每隔设定时间超时一次。
        oldValue：不为NULL时则返回定时器这次设置之前的超时时间。

     */
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
        std::cout << "timerfd_settime()" << std::endl;
    }
}

void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    std::cout << "TimerQueue::handleRead() " << howmany << " at " << now.toString() << std::endl;
    if (n != sizeof howmany)
    {
        std::cout  << "TimerQueue::handleRead() reads " << n << " bytes instead of 8" << std::endl;
    }
}


//注意fd和channel都是它创建的
TimerQueue::TimerQueue(EventLoop *loop)
  : loop_(loop),
    timerfd_(createTimerfd()),
    timerfdChannel_(loop, timerfd_),
    timers_()
{
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    ::close(timerfd_);
    //使用了unique-ptr
    // do not remove channel, since we're in EventLoop::dtor();
    for (TimerList::iterator it = timers_.begin();
         it != timers_.end(); ++it)
    {
        delete it->second;
    }
}

TimerId TimerQueue::addTimer(const TimerCallback& cb,
                             Timestamp when,
                             double interval)
{
    Timer* timer = new Timer(cb, when, interval);
    loop_->runInLoop(
            std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer);
}

void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop(
            std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);

    if (earliestChanged)
    {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if (it != activeTimers_.end())
    {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1); (void)n;
        delete it->first; // FIXME: no delete please
        activeTimers_.erase(it);
    }
    else if (callingExpiredTimers_)
    {
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}




bool TimerQueue::insert(Timer *timer) {
    bool earliestChanged = false;
    assert(timers_.size() == activeTimers_.size());
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if(it == timers_.end() || when < it->first) {
        earliestChanged = true;
    }
    {
        std::pair<TimerList::iterator, bool> result =
                timers_.insert(std::make_pair(when, timer));
        assert(result.second);
        (void) result;
    }
    {
        std::pair<ActiveTimerSet::iterator, bool> result
                = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second); (void)result;
    }
    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}

void TimerQueue::handleRead() {
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);
    std::vector<Entry> expired = getExpired(now);
    // safe to callback outside critical section
    for (std::vector<Entry>::iterator it = expired.begin();
         it != expired.end(); ++it)
    {
        it->second->run();
    }

    reset(expired, now);
}
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now) {
    std::vector<Entry> expired;
    Entry sentry = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    //timerlist是set，是有序排列的
    TimerList::iterator it = timers_.lower_bound(sentry);
    assert(it == timers_.end() || now < it->first);
    //使用push_back的插入迭代器
    std::copy(timers_.begin(), it, back_inserter(expired));
    timers_.erase(timers_.begin(), it);
    for(auto entry: expired) {
        ActiveTimer timer(entry.second, entry.second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1); (void)n;
    }
    assert(timers_.size() == activeTimers_.size());
    return expired;
}


//都存在一些误差吧
void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now) {
    for(std::vector<Entry>::const_iterator it = expired.begin(); it != expired.end(); it++) {
        //timer类型是重复的还是一次性的
        if(it->second->repeat()) {
            it->second->restart(now);
            insert(it->second);
        }
        else {
            delete it->second;
        }
        Timestamp nextExpire;
        //重新插入timer后，可能需要更新timefd的下个定时时间
        if(!timers_.empty()) {
            nextExpire = timers_.begin()->second->expiration();
        }

        if (nextExpire.valid()) {
            resetTimerfd(timerfd_, nextExpire);
        }
    }
}