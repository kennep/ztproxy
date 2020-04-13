#ifndef ZTPROXY_CONFIG_H_INCLUDED
#define ZTPROXY_CONFIG_H_INCLUDED

#include <string>
#include <cstdint>

#include "utils.h"

namespace config
{
    utils::path get_config_path(uint64_t network_id);
    utils::path get_config_home();
}

#endif
