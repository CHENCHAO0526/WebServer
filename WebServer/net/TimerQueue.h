//
// Created by cc on 8/5/21.
//

#ifndef CC_WEBSERVER_MUDUO_TIMERQUEUE_H
#define CC_WEBSERVER_MUDUO_TIMERQUEUE_H
#include "memory"
#include "Timestamp.h"
#include <set>
#include <vector>
#include "Callbacks.h"
#include "Channel.h"
class EventLoop;
class Timer;
class TimerId;


class TimerQueue
{
public:
    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    ///
    /// Schedules the callback to be run at given time,
    /// repeats if @c interval > 0.0.
    ///
    /// Must be thread safe. Usually be called from other threads.
    TimerId addTimer(const TimerCallback& cb,
                     Timestamp when,
                     double interval);

    void cancel(TimerId timerId);
private:
//为了区分timerstamp相同的
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;
//为什么activetimer又是这样的pair,timer*加上sequence_(全局第n个timer)
 //   为什么要使用timeid: timeid是和activateTimer相匹配的,本身timer就可有&timer和sequence确定，使用在timequeue中使用timestamp是因为
 //   timequeue需要对timestamp排序
 // 不能都使用timestamp，因为timestamp会变，timerid才是不变的标识
    typedef std::pair<Timer*, int64_t> ActiveTimer;
    typedef std::set<ActiveTimer> ActiveTimerSet;


    void addTimerInLoop(Timer* timer);
    void cancelInLoop(TimerId timerId);
    void handleRead();
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    bool insert(Timer* timer);

    EventLoop* loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    // Timer list sorted by expiration
    TimerList timers_;

    // for cancel()
    bool callingExpiredTimers_; /* atomic */
    ActiveTimerSet activeTimers_;
    ActiveTimerSet cancelingTimers_;

};

#endif //CC_WEBSERVER_MUDUO_TIMERQUEUE_H
