#include "zt_manager.h"
#include "config.h"
#include "utils.h"

#include <iostream>
#include <iomanip>
#include <stdexcept>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ZeroTier.h>

using namespace std;

condition_variable
ztproxy::zt_manager::started_condition;

mutex
ztproxy::zt_manager::state_mutex;

ztproxy::zt_manager *
ztproxy::zt_manager::instance;

ztproxy::zt_manager::zt_manager(uint64_t network_id)
{
    lock_guard lk(state_mutex);
    if(instance != NULL)
    {
        throw logic_error("zt_manager can only be instantiated once");
    }
    instance = this;
    this->network_id = network_id;

    started = false;
}

void
ztproxy::zt_manager::start()
{
    unique_lock lk(state_mutex);
    if(started)
    {
        throw logic_error("zt_manager already started");
    }
    auto config_path = config::get_config_path(network_id);
    cout << "Config path: " << config_path << endl;

    cout << "Starting ZeroTier service..." << endl;
    zts_start(config_path.c_str(), zt_callback_dispatcher, 9994);
    started_condition.wait(lk);
    cout << "ZeroTier service started." << endl;
    cout << "Joining network " << utils::hex(network_id) << endl;
    zts_join(network_id);
}


void
ztproxy::zt_manager::zt_callback_dispatcher(zts_callback_msg *msg)
{
    instance->zt_callback(msg);
}

void
ztproxy::zt_manager::zt_callback(zts_callback_msg *msg)
{
    switch(msg->eventCode)
    {
        case ZTS_EVENT_NODE_UP:
            cerr << "ZTS_EVENT_NODE_UP" << endl;
            break;
        case ZTS_EVENT_STACK_UP:
            cerr << "ZTS_EVENT_STACK_UP" << endl;
            break;
        case ZTS_EVENT_PEER_P2P:
            cerr << "ZTS_EVENT_PEER_P2P" << endl;
            break;
        case ZTS_EVENT_PEER_RELAY:
            cerr << "ZTS_EVENT_PEER_RELAY" << endl;
            break;
        case ZTS_EVENT_NETIF_UP:
            cerr << "ZTS_EVENT_NETIF_UP, mac=" << utils::hex(msg->netif->mac) << endl;
            break;
        case ZTS_EVENT_ADDR_ADDED_IP4:
            {
                struct sockaddr_in *in4 = (struct sockaddr_in*)&(msg->addr->addr);
                loc_ipv4_address = *in4;
                cerr << "ZTS_EVENT_ADDR_ADDED_IP4 " << utils::inet_ntop(in4) << endl;
            }
            break;
        case ZTS_EVENT_ADDR_ADDED_IP6:
            {
                struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&(msg->addr->addr);
                loc_ipv6_address = *in6;
                cerr << "ZTS_EVENT_ADDR_ADDED_IP6 " << utils::inet_ntop(in6) << endl;
            }
            break;
        case ZTS_EVENT_NETWORK_READY_IP4:
            cerr << "ZTS_EVENT_NETWORK_READY_IP4" << endl;
            break;
        case ZTS_EVENT_NETWORK_READY_IP6:
            cerr << "ZTS_EVENT_NETWORK_READY_IP6" << endl;
            break;
        case ZTS_EVENT_NETWORK_OK:
            cerr << "ZTS_EVENT_NETWORK_OK" << endl;
            break;
        case ZTS_EVENT_NODE_ONLINE:
        {
            cerr << "ZTS_EVENT_NODE_ONLINE, node_id=" << utils::hex(msg->node->address) << endl;
            unique_lock lk(state_mutex);
            started_condition.notify_all();
            break;
        }
        default:
            cerr << "ZTS_EVENT: " << msg->eventCode << endl;
    }
}