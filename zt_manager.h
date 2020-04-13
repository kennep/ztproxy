#ifndef ZTPROXY_ZT_MANAGER_H_INCLUDED
#define ZTPROXY_ZT_MANAGER_H_INCLUDED

#include <cstdint>
#include <string>
#include <condition_variable>
#include <mutex>

#include "ip_address.h"

struct zts_callback_msg;

namespace ztproxy
{
    class zt_manager
    {   
        static std::condition_variable started_condition;
        static std::mutex state_mutex;
        static void zt_callback_dispatcher(zts_callback_msg *msg);

        static zt_manager *instance;

        uint64_t network_id;
        bool started;
        void zt_callback(zts_callback_msg *msg);
        ipv4_address loc_ipv4_address;
        ipv6_address loc_ipv6_address;
    public:
        zt_manager(uint64_t network_id);
        void start();
        const ipv4_address &local_ipv4_address() const { return loc_ipv4_address; }
        const ipv6_address &local_ipv6_address() const { return loc_ipv6_address; }
    };
}

#endif
