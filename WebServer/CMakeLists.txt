set(SRC
        main.cpp
        #Epoll.cpp
        )

add_executable(WebServer ${SRC})


include_directories(${PROJECT_SOURCE_DIR}/WebServer/base)
#target_include_directories(WebServer
#        PRIVATE
#        ${PROJECT_SOURCE_DIR}/WebServer/base
#        )


target_link_libraries(WebServer libserver_base)
target_link_libraries(WebServer libserver_net)

add_subdirectory(base)
add_subdirectory(net)
add_subdirectory(tests)