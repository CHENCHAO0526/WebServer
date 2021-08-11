#include "Connector.h"
#include "EventLoop.h"

#include <stdio.h>

#include <iostream>
#include <functional>


void connectCallback(EventLoop* g_loop, int sockfd)
{
  printf("connected.\n");
  g_loop->quit();
}

void *connect_func(void*) {
    EventLoop loop;
    EventLoop* g_loop = &loop;
    InetAddress addr("127.0.0.1", 9981);
    ConnectorPtr connector(new Connector(&loop, addr));
    connector->setNewConnectionCallback(std::bind(&connectCallback, g_loop, std::placeholders::_1));
    connector->start();
    loop.loop();
    //pthread_detach(pthread_self());
    pthread_exit(NULL);
}



int main(int argc, char* argv[])
{
    printf("main(): pid = %d\n", getpid());
    int n=1000;
    if (argc > 1) {
        n = atoi(argv[1]);
    }
    printf("n=%d\n", n);
    std::vector<pthread_t> ids;
    ids.resize(n);
    for(size_t i=0; i < ids.size(); i++) {
        pthread_create(&ids[i], NULL, connect_func, NULL);
    }

    for(size_t i=0; i < ids.size(); i++) {
        pthread_join(ids[i], NULL);
    }


//    EventLoop loop;
//    g_loop = &loop;
//    InetAddress addr("127.0.0.1", 9981);
//    ConnectorPtr connector(new Connector(&loop, addr));
//    connector->setNewConnectionCallback(connectCallback);
//    connector->start();
//    loop.loop();


}
