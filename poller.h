#ifndef ZTPROXY_POLLER_H_INCLUDED
#define ZTPROXY_POLLER_H_INCLUDED

#include "multiplexer.h"

#include <vector>
#include <thread>
#include <mutex>
#include "poll.h"
namespace ztproxy
{
    class poller : multiplexer<int>
    {
    private:
        std::vector<pollfd> pollfds;
        std::vector<std::function<void()>> actions;
        std::thread thread;
        std::mutex mutex;
        void run();
    public:
        void register_channel(int channel, std::function<void()> action);
        void unregister_channel(int channel);

        void start();
        void join();
        static void blocking_write(int channel, const void *buffer, size_t buflen);
    };
}

#endif