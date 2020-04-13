#ifndef ZTPROXY_CONFIG_H_INCLUDED
#define ZTPROXY_CONFIG_H_INCLUDED

#include <string>
#include <filesystem>
#include <cstdint>

namespace config
{
    std::filesystem::path get_config_path(uint64_t network_id);
    std::filesystem::path get_config_home();
}

#endif
