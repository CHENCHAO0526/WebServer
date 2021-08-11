

link_libraries(libserver_base)
link_libraries(libserver_net)

add_executable(test1_eventloop
        test1_eventloop.cpp)

#add_executable(test3_timefd
#        test3_timefd.cpp)

add_executable(test4_timequeue
        test4_timequeue.cpp)

add_executable(test7_acceptor
        test7_acceptor.cc)

add_executable(test8_tcpserver
        test8_tepserver.cc)

add_executable(test9_echo
        test9_echo.cc)

add_executable(test10_sendlen
        test10_sendlen.cc)

add_executable(test11_threadpool
        test11_threadpool.cc)

add_executable(test12_connector
        test12_connector.cc)


add_executable(test13_tcpclient
        test13_tcpclient.cc)

include_directories(
        ${PROJECT_SOURCE_DIR}/WebServer/base
        )
include_directories(
        ${PROJECT_SOURCE_DIR}/WebServer/net
)



#target_link_libraries(Test_1_eventloop libserver_base)
#target_link_libraries(Test_1_eventloop libserver_net)
#
#target_link_libraries(Test3_timefd libserver_base)
#target_link_libraries(Test3_timefd libserver_net)