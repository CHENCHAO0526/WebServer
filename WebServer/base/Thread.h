//
// Created by cc on 8/4/21.
//

#ifndef CC_WEBSERVER_MUDUO_THREAD_H
#define CC_WEBSERVER_MUDUO_THREAD_H

#include "noncopyable.h"
#include "pthread.h"
#include "string"
#include "functional"
#include "memory"



class Thread : noncopyable
{
public:
    typedef std::function<void ()> ThreadFunc;

    explicit Thread(const ThreadFunc&, const std::string& name = std::string());
    // FIXME: make it movable in C++11
    ~Thread();

    void start();
    void join(); // return pthread_join()

    bool started() const { return started_; }
    // pthread_t pthreadId() const { return pthreadId_; }
    pid_t tid() const { return *tid_; }
    const std::string& name() const { return name_; }

// static int numCreated() { return numCreated_.get(); }

private:
    void setDefaultName();

    bool       started_;
    bool       joined_;
    pthread_t  pthreadId_;
    std::shared_ptr<pid_t>   tid_;
    ThreadFunc func_;
    std::string     name_;
//    CountDownLatch latch_;
//记录线程
//static AtomicInt32 numCreated_;
};


//一个命名空间，用来记录当前线程或者所某个线程的pid_t等信息
namespace CurrentThread {
    pid_t tid();
    const char* name();
    bool isMainThread();
}

#endif //CC_WEBSERVER_MUDUO_THREAD_H
