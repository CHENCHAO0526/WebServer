//
// Created by cc on 8/6/21.
//

#ifndef CC_WEBSERVER_MUDUO_SOCKET_H
#define CC_WEBSERVER_MUDUO_SOCKET_H

#include "noncopyable.h"

class InetAddress;
class Socket : noncopyable {
public:
    explicit Socket(int sockfd)
      : sockfd_(sockfd)
      { }

    ~Socket();

    int fd() const { return sockfd_; }

    /// abort if address in use
    void bindAddress(const InetAddress& localaddr);
    /// abort if address in use
    void listen();

    /// On success, returns a non-negative integer that is
    /// a descriptor for the accepted socket, which has been
    /// set to non-blocking and close-on-exec. *peeraddr is assigned.
    /// On error, -1 is returned, and *peeraddr is untouched.
    int accept(InetAddress* peeraddr);

    ///
    /// Enable/disable SO_REUSEADDR
    ///
    void setReuseAddr(bool on);

    void shutdownWrite();

    ///
    /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
    ///
    void setTcpNoDelay(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};


#endif //CC_WEBSERVER_MUDUO_SOCKET_H
