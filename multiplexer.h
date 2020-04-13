#ifndef ZTPROXY_MULTIPLEXER_H_INCLUDED
#define ZTPROXY_MULTIPLEXER_H_INCLUDED

#include <functional>

namespace ztproxy
{
    template<typename ChannelType> class multiplexer {
    public:
        virtual void register_channel(ChannelType channel, std::function<void()> action) = 0;
        virtual void unregister_channel(ChannelType channel) = 0;

        virtual void start() = 0;
        virtual void join() = 0;
    };
}

#endif
