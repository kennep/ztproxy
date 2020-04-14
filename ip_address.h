#ifndef ZTPROXY_IP_ADDRESS_H_INCLUDED
#define ZTPROXY_IP_ADDRESS_H_INCLUDED
#include <string>
#include <memory>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <ZeroTier.h>

namespace ztproxy
{

    class ip_address {
    public:
        virtual std::string address() const = 0;
        virtual operator sockaddr *() const = 0;
        virtual int family() const = 0;
        virtual int port() const  = 0;
        virtual int socktype() const = 0;
        virtual int zts_family() const = 0;
        virtual zts_sockaddr_storage zts_sockaddr() const = 0;
        virtual int zts_addrlen() const = 0;
        virtual ~ip_address() {}
        static std::unique_ptr<ip_address> from_sockaddr(const sockaddr &addr);
        static std::unique_ptr<ip_address> from_addrinfo(const addrinfo &addr);
        static std::vector<std::unique_ptr<ip_address>> lookup(const std::string hostname, const std::string service);
    };

    class ipv4_address : public ip_address {
    private:
        sockaddr_in addr;
        int socktyp;
    public:
        ipv4_address();
        ipv4_address(const sockaddr_in &addr, int socktype=0);
        std::string address() const override;
        operator sockaddr *() const override;  
        operator sockaddr_in *() const;
        int family() const override { return AF_INET; }
        int port() const override { return htons(addr.sin_port); }
        int socktype() const override { return socktyp; }
        int zts_family() const override { return ZTS_AF_INET; }
        zts_sockaddr_storage zts_sockaddr() const override;
        int zts_addrlen() const override { return sizeof(zts_sockaddr_in); }
    };

    class ipv6_address : public ip_address {
    private:
        sockaddr_in6 addr;
        int socktyp;
    public:
        ipv6_address();
        ipv6_address(const sockaddr_in6 &addr, int socktype=0);
        std::string address() const override;  
        operator sockaddr *() const override;  
        operator sockaddr_in6*() const;
        int family() const override { return AF_INET6; }
        int port() const override { return htons(addr.sin6_port); }
        int socktype() const override { return socktyp; }
        int zts_family() const override { return ZTS_AF_INET6; }
        zts_sockaddr_storage zts_sockaddr() const override;
        int zts_addrlen() const override { return sizeof(zts_sockaddr_in6); }
    };
}

#endif