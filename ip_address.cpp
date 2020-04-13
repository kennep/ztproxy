#include "ip_address.h"
#include "utils.h"

#include <stdexcept>

#include <string.h>

using namespace std;

ztproxy::ipv4_address::ipv4_address()
{
    addr.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    socktyp = 0;
}

ztproxy::ipv4_address::ipv4_address(const sockaddr_in &addr, int socktype)
{
    memcpy(&(this->addr), &addr, sizeof(sockaddr_in));
    socktyp = socktype;
}

std::string
ztproxy::ipv4_address::address() const
{
    return utils::inet_ntop(&addr);
}

ztproxy::ipv4_address::operator sockaddr *() const
{
    return (sockaddr *)&addr;
}

ztproxy::ipv4_address::operator sockaddr_in *() const
{
    return (sockaddr_in *)&addr;
}

zts_sockaddr_storage
ztproxy::ipv4_address::zts_sockaddr() const
{
    zts_sockaddr_storage rv;
    zts_sockaddr_in *s = (zts_sockaddr_in *)&rv;
    s->sin_family = addr.sin_family;
    s->sin_port = addr.sin_port;
    s->sin_len = sizeof(zts_sockaddr_in);
    memcpy(&(s->sin_addr), &(addr.sin_addr), sizeof(addr.sin_addr));
    return rv;
}


ztproxy::ipv6_address::ipv6_address()
{
    addr.sin6_addr = IN6ADDR_LOOPBACK_INIT;
    addr.sin6_family = AF_INET6;
    addr.sin6_port = 0;
    socktyp = 0;
}

ztproxy::ipv6_address::ipv6_address(const sockaddr_in6 &addr, int socktype)
{
    memcpy(&(this->addr), &addr, sizeof(sockaddr_in6));
    socktyp = socktype;
}

std::string
ztproxy::ipv6_address::address() const
{
    return utils::inet_ntop(&addr);
}

ztproxy::ipv6_address::operator sockaddr *() const
{
    return (sockaddr *)&addr;
}

ztproxy::ipv6_address::operator sockaddr_in6 *() const
{
    return (sockaddr_in6 *)&addr;
}

zts_sockaddr_storage
ztproxy::ipv6_address::zts_sockaddr() const
{
    zts_sockaddr_storage rv;
    zts_sockaddr_in6 *s = (zts_sockaddr_in6 *)&rv;
    s->sin6_family = addr.sin6_family;
    s->sin6_port = addr.sin6_port;
    s->sin6_len = sizeof(zts_sockaddr_in6);
    memcpy(&(s->sin6_addr), &(addr.sin6_addr), sizeof(addr.sin6_addr));
    return rv;
}

unique_ptr<ztproxy::ip_address>
ztproxy::ip_address::from_sockaddr(const sockaddr &addr)
{
    switch(addr.sa_family)
    {
    case AF_INET:
        return unique_ptr<ztproxy::ip_address>(new ipv4_address((const sockaddr_in &)addr));
    case AF_INET6:
        return unique_ptr<ztproxy::ip_address>(new ipv6_address((const sockaddr_in6 &)addr));
    default:
        throw invalid_argument("Unsupported address family: " + std::to_string(addr.sa_family));
    }
}

unique_ptr<ztproxy::ip_address>
ztproxy::ip_address::from_addrinfo(const addrinfo &addr)
{
    switch(addr.ai_family)
    {
    case AF_INET:
        return unique_ptr<ztproxy::ip_address>(new ipv4_address((const sockaddr_in &)(*addr.ai_addr), addr.ai_socktype));
    case AF_INET6:
        return unique_ptr<ztproxy::ip_address>(new ipv6_address((const sockaddr_in6 &)(*addr.ai_addr), addr.ai_socktype));
    default:
        throw invalid_argument("Unsupported address family: " + std::to_string(addr.ai_family));
    }
}

class getaddrinfo_error_category : public error_category
{
    const char *name() const noexcept { return "getaddrinfo"; }
    std::string message(int condition) const { return ::gai_strerror(condition); }

};

getaddrinfo_error_category getaddrinfo_error_category_instance;

const error_category&
getaddrinfo_category()
{
    return getaddrinfo_error_category_instance;
}

std::vector<std::unique_ptr<ztproxy::ip_address>>
ztproxy::ip_address::lookup(const std::string hostname, const std::string service)
{
    vector<std::unique_ptr<ztproxy::ip_address>> retval;
    struct addrinfo *res;
    struct addrinfo hints;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;
    hints.ai_protocol = 0;
    int rv = ::getaddrinfo(hostname.c_str(), service.c_str(), &hints, &res);
    if(rv!=0)
    {
        throw system_error(rv, getaddrinfo_category(), "getaddrinfo");
    }
    struct addrinfo *cur = res;
    do
    {
        retval.push_back(from_addrinfo(*cur));
    } while(cur=cur->ai_next, cur != NULL);

    ::freeaddrinfo(res);
    return retval;

}