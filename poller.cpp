#include "poller.h"
#include "utils.h"

#include <exception>
#include <iostream>

#include <unistd.h>

using namespace std;

void
ztproxy::poller::register_channel(int channel, function<void()> action)
{
    lock_guard<std::mutex> lk(mutex);
    pollfd pollfd;
    pollfd.fd = channel;
    pollfd.events = POLLIN;

    pollfds.emplace_back(pollfd);
    actions.emplace_back(action);
}

void
ztproxy::poller::unregister_channel(int channel)
{
    cerr << "poller: unregistering channel: " << channel << endl;
    lock_guard<std::mutex> lk(mutex);
    for(decltype(pollfds)::size_type i=0; i<pollfds.size();++i)
    {
        if(pollfds[i].fd == channel) {
            pollfds.erase(pollfds.begin() + i);
            actions.erase(actions.begin() + i);
        }
    }
    cerr << "poller: unregistered channel: " << channel << endl;
}

void
ztproxy::poller::run()
{
    while(true)
    {
        vector<function<void()>> pending_actions;
        {
            lock_guard<std::mutex> lk(mutex);
            if(::poll(pollfds.data(), pollfds.size(), 0)==-1)
            {
                utils::throw_errno("poll");
            }
            for(decltype(pollfds)::size_type i=0; i<pollfds.size(); ++i)
            {
                if(pollfds[i].revents & POLLIN) {
                    pending_actions.emplace_back(actions[i]);
                }
            }
        }

        for(auto it = pending_actions.begin(); it != pending_actions.end(); ++it)
        {
            try
            {
                (*it)();
            }
            catch(exception &e)
            {
                cerr << "ERROR: " << e.what() << endl;
            }
        }
    }
}

void
ztproxy::poller::start()
{
    thread = std::thread([this](){this->run();});
}

void
ztproxy::poller::join()
{
    thread.join();
}

void
ztproxy::poller::blocking_write(int fd, const void *buffer, size_t len)
{
    while(true)
    {
        int nbytes = ::write(fd, buffer, len);
        if(nbytes <0)
        {
            if(errno == EINTR||errno == EAGAIN||errno == EWOULDBLOCK)
            {
                pollfd pollfd;
                pollfd.fd = fd;
                pollfd.events = POLLOUT;
                ::poll(&pollfd, 1, -1);
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