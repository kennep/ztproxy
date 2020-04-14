#ifndef ZTPROXY_PROXY_MANAGER_H_INCLUDED
#define ZTPROXY_PROXY_MANAGER_H_INCLUDED

#include <list>
#include <vector>
#include <string>

#include "zt_manager.h"

namespace ztproxy {
    class proxy_manager;

    class proxy {
    private:
        proxy_manager &manager;

        int listening_port;
        std::string target_host;
        std::string target_port;
        int listening_socket;

        int make_socket();
        void set_nonblock(int socket);
        void accept();
        void accept_thread();
        bool read_local(int local_sock, int remote_sock);
        void read_local_thread(int local_sock, int remote_sock);
        bool read_remote(int local_sock, int remote_sock);
        void read_remote_thread(int local_sock, int remote_sock);
        void close_local(std::string reason, int local_sock);
        void close_remote(std::string reason, int remote_sock);
        void write_all_local(int sock, void *buf, size_t buflen);
        void write_all_remote(int sock, void *buf, size_t buflen);
        bool is_transient_error();
    public:
        proxy(proxy_manager &manager, const std::string &target_spec);
        void listen();
    };

    class proxy_manager {
        friend class proxy;
    private:
        std::list<proxy> proxies;

        bool use_ipv4;
        bool use_ipv6;
    public:
        proxy_manager(bool use_ipv4, bool use_ipv6);
        void add_proxy(const std::string &target_spec);
        void main();
    };
}

#endif