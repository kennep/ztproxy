#include "command_server.h"

#include "config.h"
#include "proxy_manager.h"

#include <iostream>

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

ztproxy::command_server::command_server(uint64_t network_id, ztproxy::proxy_manager &mgr) : proxy_mgr(mgr)
{
    this->network_id = network_id;
}

void
ztproxy::command_server::run()
{
    auto config_path = config::get_config_path(network_id);
    auto port_filename = config_path / "ztproxy.port";

    int sock;
    int yes = 1;
    struct sockaddr_in name;

    /* Create the socket. */
    sock = ::socket (PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        utils::throw_errno("socket");
    }

    if(::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        ::close(sock);
        utils::throw_errno("setsockopt");
    }

    /* Give the socket a name. */
    name.sin_family = AF_INET;
    name.sin_port = 0;
    name.sin_addr.s_addr = ::htonl (INADDR_LOOPBACK);
    auto close_sock = [sock](){ ::close(sock); };

    if (::bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0) {
        utils::throw_errno("bind", close_sock);
    }

    socklen_t namelen = sizeof(name);
    if (::getsockname(sock, (sockaddr *)&name, &namelen)< 0)
    {
        utils::throw_errno("getsockname", close_sock);
    }

    if (::listen (sock, 1) < 0) {
        utils::throw_errno("listen", close_sock);
    }

    listening_port = ::ntohs(name.sin_port);
    listening_socket = sock;

    int fd = ::open(port_filename.c_str(), O_WRONLY|O_CREAT|O_EXCL, 00600);
    if(fd == -1)
    {
        utils::throw_errno(std::string("open ") + port_filename.c_str(), close_sock);
    }

    std::string portstr = std::to_string(listening_port) + "\n";
    auto rv = ::write(fd, portstr.c_str(), portstr.size());
    if(rv < (ssize_t)portstr.size())
    {
       utils::throw_errno(std::string("write ") + port_filename.c_str(), 
        [sock, fd](){ ::close(sock); ::close(fd); });
    }
    ::close(fd);

    while(true)
    {
        accept();
    }
}

void
ztproxy::command_server::accept()
{
    struct sockaddr_in sockaddr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int local_sock = ::accept(listening_socket, (struct sockaddr *)&sockaddr, &addrlen);
    if(local_sock == -1) {
        utils::throw_errno();
    }

    std::string line;
    writeline(local_sock, "READY.");
    while(readline(local_sock, line))
    {
        auto args = utils::split_string(line, " ", 2);
        if(args.size() != 2)
        {
            writeline(local_sock, "?SYNTAX ERROR");
            continue;
        }
        auto command = args[0];
        auto arg = args[1];
        try 
        {
            if(command == "PROXY")
            {
                proxy_mgr.add_proxy(arg);
                writeline(local_sock, "OK");
            }
            else
            {
                writeline(local_sock, "?INVALID COMMAND");
            }
            
        }
        catch(std::exception &e)
        {
            writeline(local_sock, std::string("?ERROR: ") + e.what());
        }
    }
}

bool
ztproxy::command_server::readline(int &sock, std::string &output)
{
    while(sock != -1)
    {
        const int MAXMSG=8192;
        char buffer[MAXMSG];
        int nbytes;

        nbytes = ::read (sock, buffer, MAXMSG);
        if (nbytes < 0)
        {
            if(is_transient_error()) return true;
            /* Read error. */
            ::perror ("read");
            ::close(sock);
            sock = -1;
            nbytes = 0;
        }
        else if (nbytes == 0)
        {
            /* End-of-file. */
            ::close(sock);
            sock = -1;
        }

        if(nbytes > 0) read_buffer += std::string(buffer, nbytes);
        auto pos = read_buffer.find("\n");
        if(pos != std::string::npos)
        {
            output = read_buffer.substr(0, pos);
            read_buffer = read_buffer.substr(pos + 1);
            return sock != -1;
        }
    }
    return sock != -1;

}

void
ztproxy::command_server::writeline(int sock, const std::string &line) const
{
    std::string l = line + "\n";
    if(::write(sock, l.c_str(), l.size())==-1)
    {
        utils::throw_errno();
    }
}

bool
ztproxy::command_server::is_transient_error()
{
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR;
}