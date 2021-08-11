//
// Created by cc on 8/6/21.
//

#ifndef CC_WEBSERVER_MUDUO_INETADDRESS_H
#define CC_WEBSERVER_MUDUO_INETADDRESS_H
#include "copyable.h"
#include <string>

#include <netinet/in.h>

namespace sockets
{
    const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
}

///
/// Wrapper of sockaddr_in.
///
/// This is an POD interface class.
class InetAddress : copyable
{
public:
    /// Constructs an endpoint with given port number.
    /// Mostly used in TcpServer listening.
    explicit InetAddress(uint16_t port);

    /// Constructs an endpoint with given ip and port.
    /// @c ip should be "1.2.3.4"
    InetAddress(const std::string& ip, uint16_t port);

    /// Constructs an endpoint with given struct @c sockaddr_in
    /// Mostly used when accepting new connections
    InetAddress(const struct sockaddr_in& addr)
            : addr_(addr)
    { }


    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t port() const;

    //const struct sockaddr* getSockAddr() const { return sockets::sockaddr_cast(&addr6_); }

    std::string toHostPort() const;

    // default copy/assignment are Okay
    const struct sockaddr_in& getSockAddrInet() const { return addr_; }
    void setSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr; }

private:
    union
    {
        struct sockaddr_in addr_;
        struct sockaddr_in6 addr6_;
    };
    //struct sockaddr_in6 addr6_;
};

#endif //CC_WEBSERVER_MUDUO_INETADDRESS_H
