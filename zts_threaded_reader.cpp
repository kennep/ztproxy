#include "zts_threaded_reader.h"
#include "utils.h"

#include <exception>
#include <iostream>
#include <thread>

#include <ZeroTier.h>

using namespace std;

void
ztproxy::zts_threaded_reader::register_channel(int channel, function<void()> action)
{
    lock_guard<std::mutex> lk(mutex);
    channels.emplace_back(channel, action);

    cerr << "zts_selector: channel registered: " << channel << endl;
}

void
ztproxy::zts_threaded_reader::unregister_channel(int channel)
{
    cerr << "zts_selector: unregistering channel: " << channel << endl;
    unique_lock<std::mutex> lk(mutex);
    for(auto it = channels.begin(); it != channels.end(); ++it)
    {
        auto &elem = (*it);
        if(elem.fd == channel) {
            ::zts_close(elem.fd);
            cerr << "Cancel for channel " << &elem << " " << elem.fd;
            elem.cancel();
            channels.erase(it);
            lk.unlock();
            break;
        }
        
    }
    cerr << "zts_selector: unregistered channel: " << channel << endl;
}

void
ztproxy::zts_threaded_reader::channel_info::run()
{
    cerr << "Begin channel " << this << " " << fd << endl;
    while(!cancellation_token.is_cancelled())
    {
        cerr << "Run loop for channel " << this << " " << fd << endl;
        fd_set read_set;
        int max_fd = fd;
        FD_ZERO(&read_set);
        FD_SET(fd, &read_set);
        if(::zts_select(max_fd + 1, &read_set, NULL, NULL, NULL)==-1)
        {
                utils::throw_errno("zts_select");
        }
        if(FD_ISSET(fd, &read_set))
        {
            try {
               action();
            }
            catch(exception &e)
            {
                cerr << "ERROR: " << e.what() << endl;
            }
        }
    }
    cerr << "Run loop completed for channel " << this << " " << fd << endl;
}

void
ztproxy::zts_threaded_reader::start()
{
}

void
ztproxy::zts_threaded_reader::join()
{
}

void
ztproxy::zts_threaded_reader::blocking_write(int fd, const void *buffer, size_t len)
{
    while(true)
    {
        int nbytes = ::zts_write(fd, buffer, len);
        if(nbytes < 0)
        {
            if(errno == EINTR||errno == EAGAIN||errno == EWOULDBLOCK)
            {
                fd_set write_set;
                FD_ZERO(&write_set);
                FD_SET(fd, &write_set);
                ::zts_select(fd + 1, NULL, &write_set, NULL, NULL);
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
