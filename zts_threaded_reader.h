#ifndef ZTPROXY_ZTS_THREADED_READER_H_INCLUDED
#define ZTPROXY_ZTS_THREADED_READER_H_INCLUDED

#include "multiplexer.h"
#include "zt_manager.h"
#include "utils.h"

#include <list>
#include <thread>
#include <mutex>

namespace ztproxy
{
    class zts_threaded_reader : multiplexer<int>
    {
    private:
        class channel_info
        {
        public:
            channel_info(int fd, std::function<void()> action) : thread([this](){this->run();})
            {
                this->fd = fd;
                this->action = action;
                thread.detach();
            }

            int fd;
            void cancel() { cancellation_token.cancel(); }
        private:
            std::function<void()> action;
            std::thread thread;
            utils::cancellation_token cancellation_token;
            void run();
        };

        std::list<channel_info> channels;
        std::thread thread;
        std::mutex mutex;
    public:
        virtual void register_channel(int channel, std::function<void()> action);
        virtual void unregister_channel(int channel);

        virtual void start();
        virtual void join();
        static void blocking_write(int channel, const void *buffer, size_t buflen);
    };
}

#endif