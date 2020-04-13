#include "zts_select.h"
#include "utils.h"

#include <exception>
#include <iostream>

#include <ZeroTier.h>

using namespace std;

void
ztproxy::zts_selecter::register_channel(int channel, function<void()> action)
{
    cerr << "zts_selector: register_channel: " << channel << endl;
    lock_guard<std::mutex> lk(mutex);
    fds_to_actions.emplace_back(channel, action);
    int client_control_fd = ::zts_socket(ZTS_AF_INET, ZTS_SOCK_DGRAM, 0);
    const sockaddr_in *local_addr = zt_mgr.local_ipv4_address();
    zts_sockaddr_in addr;
    addr.sin_len = sizeof(zts_sockaddr_in);
    addr.sin_port = ::htons(control_port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = local_addr->sin_addr.s_addr;
    const char *msg = "!";
    if(::zts_sendto(client_control_fd, msg, 1, 0, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        utils::throw_errno("zts_sendto");
    }

    cerr << "zts_selector: channel registered. " << channel << endl;
}

void
ztproxy::zts_selecter::unregister_channel(int channel)
{
    lock_guard<std::mutex> lk(mutex);
    for(decltype(fds_to_actions)::size_type i=0; i<fds_to_actions.size();++i)
    {
        if(fds_to_actions[i].first == channel) {
            fds_to_actions.erase(fds_to_actions.begin() + i);
            break;
        }
    }
}

void
ztproxy::zts_selecter::run()
{
    
    control_fd = ::zts_socket(ZTS_AF_INET, ZTS_SOCK_DGRAM, 0);
    if(control_fd < 0)
    {
        utils::throw_errno("zts_socket");
    }
    zts_sockaddr_in addr;
    const sockaddr_in *local_addr = zt_mgr.local_ipv4_address();
    addr.sin_len = sizeof(zts_sockaddr_in);
    addr.sin_port = 0;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = local_addr->sin_addr.s_addr;
    if (::zts_bind(control_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        utils::throw_errno("zts_bind");
    }
    socklen_t namelen = sizeof(addr);
    if (::zts_getsockname(control_fd, (struct sockaddr *) &addr, &namelen) < 0)
    {
        utils::throw_errno("zts_getsockname");
    }
    control_port = ::ntohs(addr.sin_port);

    while(true)
    {
        vector<function<void()>> pending_actions;
        {
            unique_lock<std::mutex> lk(mutex);
            fd_set read_set;
            int max_fd = control_fd;
            FD_ZERO(&read_set);
            FD_SET(control_fd, &read_set);
            cerr << "#Registered channels: " << fds_to_actions.size() << endl;
            for(auto it = fds_to_actions.begin(); it != fds_to_actions.end(); ++it)
            {
                cerr << "zts_selector: fd: " << (*it).first << endl;
                FD_SET((*it).first, &read_set);
                if((*it).first > max_fd) max_fd = (*it).first;
            }
            
            lk.unlock();
            if(::zts_select(max_fd + 1, &read_set, NULL, NULL, NULL)==-1)
            {
                utils::throw_errno("zts_select");
            }
            if(FD_ISSET(control_fd, &read_set))
            {
                char msg[256];
                zts_sockaddr_in addr;
                socklen_t namelen = sizeof(addr);
                ::zts_recvfrom(control_fd, msg, 256, 0, (sockaddr *)&addr, &namelen); // Ingnore errors
            }

            lk.lock();
            for(auto it = fds_to_actions.begin(); it != fds_to_actions.end(); ++it)
            {
                if(FD_ISSET((*it).first, &read_set))
                {
                    pending_actions.emplace_back((*it).second);
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
ztproxy::zts_selecter::start()
{
    thread = std::thread([this](){this->run();});
}

void
ztproxy::zts_selecter::join()
{
    thread.join();
}

void
ztproxy::zts_selecter::blocking_write(int fd, const void *buffer, size_t len)
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
                ::select(fd + 1, NULL, &write_set, NULL, NULL);
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
