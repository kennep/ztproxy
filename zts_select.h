#ifndef ZTPROXY_ZT_SELECT_H_INCLUDED
#define ZTPROXY_ZT_SELECT_H_INCLUDED

#include "multiplexer.h"
#include "zt_manager.h"

#include <vector>
#include <thread>
#include <mutex>

namespace ztproxy
{
    class zts_selecter : multiplexer<int>
    {
    private:
        int control_fd;
        int control_port;
        std::vector<std::pair<int, std::function<void()>>> fds_to_actions;
        std::thread thread;
        std::mutex mutex;
        void run();
        const zt_manager &zt_mgr;
    public:
        zts_selecter(const zt_manager &zt_manager) : zt_mgr(zt_manager) {}
        virtual void register_channel(int channel, std::function<void()> action);
        virtual void unregister_channel(int channel);

        virtual void start();
        virtual void join();
        void blocking_write(int channel, const void *buffer, size_t buflen);
    };
}

#endif