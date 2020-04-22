#ifndef ZTPROXY_COMMAND_SERVER_H_INCLUDED
#define ZTPROXY_COMMAND_SERVER_H_INCLUDED

#include <string>

#include "utils.h"

namespace ztproxy
{
    class proxy_manager;
    class command_server
    {
    private:
        uint64_t network_id;
        int listening_port;
        int listening_socket;
        std::string read_buffer;
        proxy_manager &proxy_mgr;

        void accept();
        bool readline(int &sock, std::string &line);
        bool is_transient_error();
        void writeline(int sock, const std::string &line) const;
    public:
        command_server(uint64_t network_id, proxy_manager &proxy_mgr);
        void run();
    };
}

#endif