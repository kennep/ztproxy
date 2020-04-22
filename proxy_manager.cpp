#include "proxy_manager.h"
#include "utils.h"
#include "ip_address.h"

#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <thread>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>

#include <ZeroTier.h>

using namespace std;
using namespace ztproxy;

ztproxy::proxy::proxy(proxy_manager &mgr, const string &target_spec) : manager(mgr)
{
    vector<string> elements = utils::split_string(target_spec, ":");
    if(elements.size() != 3)
    {
        throw invalid_argument(target_spec + ": Target must be specificed as SRCPORT:TARGETHOST:TARGETPORT");
    }
    char *e;
    listening_port = strtol(elements[0].c_str(), &e, 10);
    if(listening_port == 0)
    {
        throw invalid_argument(target_spec + ": invalid source port. Target must be specificed as SRCPORT:TARGETHOST:TARGETPORT");
    }
    if(*e != 0)
    {
        throw invalid_argument(target_spec + ": invalid source port - trailing characters after number. Target must be specificed as SRCPORT:TARGETHOST:TARGETPORT");
    }

    target_host = elements[1];
    target_port = elements[2];

    if(target_host.empty())
    {
        throw invalid_argument(target_spec + ": target host cannot be empty");
    }
    if(target_port.empty())
    {
        throw invalid_argument(target_spec + ": target_port cannot be empty");
    }
}

int
ztproxy::proxy::make_socket()
{
    int sock;
    int yes = 1;
    struct sockaddr_in name;

    /* Create the socket. */
    sock = ::socket (PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        utils::throw_errno("socket");
    }

    if(::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        utils::throw_errno("setsockopt");
    }

    /* Give the socket a name. */
    name.sin_family = AF_INET;
    name.sin_port = ::htons (listening_port);
    name.sin_addr.s_addr = ::htonl (INADDR_LOOPBACK);
    if (::bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0) {
        utils::throw_errno("bind");
    }

    if (::listen (sock, 1) < 0) {
        utils::throw_errno("listen");
    }

    return sock;
}

void
ztproxy::proxy::set_nonblock(int socket)
{
    int flags = ::fcntl(socket, F_GETFL, 0);
    if(flags == -1) flags = 0;
    ::fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

void
ztproxy::proxy::listen()
{
    cerr << "Forwarding port " << listening_port << " to " << target_host << ":" << target_port << endl;
    listening_socket = make_socket();    
    auto t = std::thread([this](){this->accept_thread();});
    t.detach();
}

void
ztproxy::proxy::accept_thread()
{
    try {
        while(true)
        {
            accept();
        }
    }
    catch(exception & e)
    {
        cerr << "ERROR: " << e.what();
        cerr << "Stopping forwarding of " << target_host << ":" << target_port << endl;
    }
}

void
ztproxy::proxy::accept()
{
    struct sockaddr_in sockaddr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int local_sock = ::accept(listening_socket, (struct sockaddr *)&sockaddr, &addrlen);
    if(local_sock == -1) {
        if(is_transient_error()) return;
        utils::throw_errno();
    }
    string hostname;
    try {
        hostname = utils::inet_ntop(&sockaddr);
    }
    catch(system_error &e)
    {
        cerr << e.what();
        hostname = "<could not convert>";
    }
    cerr << "Accepted connection from " << hostname << ":" << sockaddr.sin_port
        << ", forwarding to " << target_host << ":" << target_port << endl;

    int remote_sock = -1;
    try
    {
        auto addrs = ztproxy::ip_address::lookup(target_host, target_port);
        for(auto it = addrs.begin(); it != addrs.end(); ++it)
        {
            try
            {
                auto &addr = (*it);
                if(addr->family() == AF_INET && !manager.use_ipv4)
                {
                    cerr << "Skipping " << addr->address() << " as IPv4 is currently disabled" << endl;
                    continue;
                }
                if(addr->family() == AF_INET6 && !manager.use_ipv6)
                {
                    cerr << "Skipping " << addr->address() << " as IPv6 is currently disabled" << endl;
                    continue;
                }
                cerr << "Connecting to: " << addr->address() << ":" << addr->port() << endl;
                remote_sock = zts_socket(addr->family() == AF_INET6 ? ZTS_AF_INET6 : ZTS_AF_INET, addr->socktype(), 0);
                if(remote_sock == -1)
                {
                    utils::throw_errno();
                }
                
                zts_sockaddr_storage zts_addr = addr->zts_sockaddr();
                if(zts_connect(remote_sock, (struct sockaddr *)&zts_addr, addr->zts_addrlen())==-1)
                {
                    utils::throw_errno();
                }
                break;
            }
            catch(system_error &e)
            {
                zts_close(remote_sock);
                remote_sock = -1;
                cerr << e.what() << endl;
            }
        }
    }
    catch(system_error &e)
    {
        cerr << e.what() << endl;
        cerr << "Closing connection." << endl;
        ::close(local_sock);
        return;
    }
    if(remote_sock == -1)
    {
        cerr << "No more target addresses to try. Closing local connection." << endl;
        ::close(local_sock);
        return;
    }


    cerr << "Local socket: " << local_sock << " Remote socket: " << remote_sock << endl;
    auto t1 = std::thread([this, local_sock, remote_sock](){this->read_local_thread(local_sock, remote_sock);});
    t1.detach();
    auto t2 = std::thread([this, local_sock, remote_sock](){this->read_remote_thread(local_sock, remote_sock);});
    t2.detach();
}

bool
ztproxy::proxy::is_transient_error()
{
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR;
}

void
ztproxy::proxy::close_local(string reason, int local_sock)
{
    cerr << reason << ": closing connection to " << target_host << ":" << target_port << endl;
    if(local_sock != -1) ::close(local_sock);
}

void
ztproxy::proxy::close_remote(string reason, int remote_sock)
{
    cerr << reason << ": closing connection from " << target_host << ":" << target_port << endl;
    if(remote_sock != -1) ::zts_close(remote_sock);
}

void
ztproxy::proxy::read_local_thread(int local_socket, int remote_socket)
{
    try {
        while(read_local(local_socket, remote_socket));
    }
    catch(exception & e)
    {
        cerr << "ERROR: " << e.what();
    }
}

bool
ztproxy::proxy::read_local(int local_sock, int remote_sock)
{
    const int MAXMSG=8192;
    char buffer[MAXMSG];
    int nbytes;

    nbytes = ::read (local_sock, buffer, MAXMSG);
    if (nbytes < 0)
    {
        if(is_transient_error()) return true;
        /* Read error. */
        ::perror ("read");
        close_local("Local read error", local_sock);
        return false;
    }
    else if (nbytes == 0)
    {
        /* End-of-file. */
        close_local("Local connection closed", local_sock);
        return false;
    }
    else
    {
        try
        {
            write_all_remote(remote_sock, buffer, nbytes);
            return true;
        }
        catch(system_error &e)
        {
            cerr << e.what();
            close_local("Remote write error", local_sock);
            return false;
        }
    }
}

void
ztproxy::proxy::read_remote_thread(int local_socket, int remote_socket)
{
    try {
        while(read_remote(local_socket, remote_socket));
    }
    catch(exception & e)
    {
        cerr << "ERROR: " << e.what();
    }
}


bool
ztproxy::proxy::read_remote(int local_sock, int remote_sock)
{
    const int MAXMSG=8192;
    char buffer[MAXMSG];
    int nbytes;

    nbytes = ::zts_read (remote_sock, buffer, MAXMSG);
    if (nbytes < 0)
    {
        if(is_transient_error()) return true;
        /* Read error. */
        ::perror ("zts_read");
        close_remote("Remote read error", remote_sock);
        return false;
    }
    else if (nbytes == 0)
    {
        /* End-of-file. */
        close_remote("Remote connection closed", remote_sock);
        return false;
    }
    else
    {
        try
        {
            write_all_local(local_sock, buffer, nbytes);
            return true;
        }
        catch(system_error &e)
        {
            cerr << e.what();
            close_remote("Local write error", remote_sock);
            return false;
        }
    }
}

void
ztproxy::proxy::write_all_local(int fd, void *buffer, size_t len)
{
    while(true)
    {
        int nbytes = ::write(fd, buffer, len);
        if(nbytes <0)
        {
            if(errno == EINTR||errno == EAGAIN||errno == EWOULDBLOCK)
            {
                continue;
            }
            else
            {
                utils::throw_errno("write");
            }            
        } else if(nbytes < (int)len)
        {
            buffer = ((char *)buffer) + nbytes;
            len -= nbytes;
        } else return;
    }
}

void
ztproxy::proxy::write_all_remote(int fd, void *buffer, size_t len)
{
    while(true)
    {
        int nbytes = ::zts_write(fd, buffer, len);
        if(nbytes <0)
        {
            if(errno == EINTR||errno == EAGAIN||errno == EWOULDBLOCK)
            {
                continue;
            }
            else
            {
                utils::throw_errno("zts_write");
            }            
        } else if(nbytes < (int)len)
        {
            buffer = ((char *)buffer) + nbytes;
            len -= nbytes;
        } else return;
    }
}

ztproxy::proxy_manager::proxy_manager(bool use_ipv4, bool use_ipv6)
{
    this->use_ipv4 = use_ipv4;
    this->use_ipv6 = use_ipv6;
}

void
ztproxy::proxy_manager::add_proxy(const string &target)
{
    proxies.emplace_back(*this, target);
    proxies.back().listen();
}

