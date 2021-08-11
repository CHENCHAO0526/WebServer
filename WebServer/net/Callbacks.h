//
// Created by cc on 8/5/21.
//

#ifndef CC_WEBSERVER_MUDUO_CALLBACKS_H
#define CC_WEBSERVER_MUDUO_CALLBACKS_H


#include "functional"
#include <memory>
//#include "Timestamp.h"

template<typename T>
inline T* get_pointer(const std::shared_ptr<T>& ptr)
{
    return ptr.get();
}

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;



typedef std::function<void()> TimerCallback;

class TcpConnection;
class Buffer;
class Timestamp;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;

typedef std::function<void (const TcpConnectionPtr&,
                              Buffer* buf,
                              Timestamp)> MessageCallback;

typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;

typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;


void defaultConnectionCallback(const TcpConnectionPtr& conn);
void defaultMessageCallback(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            Timestamp receiveTime);


#endif //CC_WEBSERVER_MUDUO_CALLBACKS_H
