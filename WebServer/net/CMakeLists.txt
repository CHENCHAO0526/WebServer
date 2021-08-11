set(NET_SRC
        Channel.cpp
        Poller.cpp
        EventLoop.cpp
        Timer.cpp
        TimerQueue.cpp
        EventLoopThread.cpp
        Acceptor.cpp
        InetAddress.cpp
        SocketsOps.cpp
        Socket.cpp
        EventLoopThread.cpp
        TcpServer.cpp
        TcpConnection.cpp
        Buffer.cpp
        EventLoopThreadPool.cpp
        Connector.cpp
        TcpClient.cpp
        )


add_library(libserver_net ${NET_SRC})

target_link_libraries(libserver_net libserver_base)


set_target_properties(libserver_net PROPERTIES OUTPUT_NAME "server_net")

#add_executable(net ${NET_SRC})
#target_link_libraries(net libserver_base)