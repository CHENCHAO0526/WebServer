//
// Created by cc on 8/4/21.
//

#include "Thread.h"
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <unistd.h>
#include "assert.h"

namespace CurrentThread
{
    __thread const char* t_threadName = "unknown";
}


//匿名空间相当于static， 想一想__thread的作用，可以用来修饰那些带有全局性且值可能变，但是又不值得用全局变量保护的变量。

namespace  {
    //cachedTid 全局唯一标识的线程号
    __thread pid_t t_cachedTid = 0;
#if !__GLIBC_PREREQ(2, 30)
    pid_t gettid() {
        //不同进程中phread id重复，这样获取真实的线程id唯一标识
        return static_cast<pid_t>(::syscall(SYS_gettid));
    }
#endif

    void afterFork()
    {
        t_cachedTid = gettid();
        CurrentThread::t_threadName = "main";
        // no need to call pthread_atfork(NULL, NULL, &afterFork);
    }

    class ThreadNameInitializer
    {
    public:
        ThreadNameInitializer()
        {
            CurrentThread::t_threadName = "main";
            pthread_atfork(NULL, NULL, &afterFork);
        }
    };
    ThreadNameInitializer init;


    //装有线程实际运行函数所需的东西，通过这个结构调用运行的函数
    struct ThreadData {
        typedef Thread::ThreadFunc ThreadFunc;
        ThreadFunc func_;
        std::string name_;
        std::weak_ptr<pid_t> wkTid_;

        ThreadData(const ThreadFunc& func,
                   const std::string& name,
                   const std::weak_ptr<pid_t>& tid)
          : func_(func),
            name_(name),
            wkTid_(tid)
        {  }

        void runInThread() {
            pid_t tid = CurrentThread::tid();
            std::shared_ptr<pid_t> sptid = wkTid_.lock();
            //TOSEE 为什么要这样呢
            if(sptid) {
                //看不懂了，赋值了又释放
                *sptid = tid;
                sptid.reset();
            }
            CurrentThread::t_threadName = name_.empty() ? "muduoThread" : name_.c_str();
            //设置进程名
            ::prctl(PR_SET_NAME, CurrentThread::t_threadName);
            func_(); // FIXME: surround with try-catch, see muduo
            CurrentThread::t_threadName = "finished";

        }
    };

    void* startThread(void* obj) {
            ThreadData* data = static_cast<ThreadData*> (obj);
            data -> runInThread();
            delete data;
            return NULL;
        }

}

pid_t CurrentThread::tid() {
    if(t_cachedTid == 0) {
        t_cachedTid = gettid();

    }
    return t_cachedTid;
}

const char* CurrentThread::name() {
    return t_threadName;
}

bool CurrentThread::isMainThread()
{
    //全局标识的线程号等于进程id
    return tid() == ::getpid();
}

//AtomicInt32 Thread::numCreated_;

Thread::Thread(const ThreadFunc& func, const std::string &name)
    : started_(false),
      joined_(false),
      pthreadId_(0),
      tid_(new pid_t(0)),
      func_(func),
      name_(name)
{
    //numCreated_.increment();
}

Thread::~Thread() {
    if(started_ && !joined_)
    {
        pthread_detach(pthreadId_);
        //TOSEE detach后需要exit?
        //pthread_exit(NULL);
    }

}

void Thread::start() {
    assert(!started_);
    started_ = true;
    ThreadData* data = new ThreadData(func_, name_, tid_);
    if(pthread_create(&pthreadId_, NULL, &startThread, data)) {
        started_ = false;
        delete data;
        abort();
    }
}


void Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    pthread_join(pthreadId_, NULL);
}
