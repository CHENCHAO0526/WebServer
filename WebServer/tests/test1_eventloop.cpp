
// Created by cc on 8/4/21.

#include "../net/EventLoop.h"
#include "Thread.h"
#include "stdio.h"
#include <unistd.h>

void threadFunc()
{
    printf("threadFunc(): pid = %d, tid = %d\n",
           getpid(), CurrentThread::tid());

    EventLoop loop;
    loop.loop();
}

int main()
{
    printf("main(): pid = %d, tid = %d\n",
           getpid(), CurrentThread::tid());

    EventLoop loop;

    Thread thread(threadFunc);
    thread.start();

    loop.loop();
    pthread_exit(NULL);
}


