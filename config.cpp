#include "config.h"
#include "utils.h"

#include <stdexcept>
#include <filesystem>
#include <sstream>
#include <iomanip>

using namespace std;

filesystem::path
config::get_config_home()
{
    auto config_home = utils::getenv("XDG_CONFIG_HOME");
    if(!config_home.has_value())
    {
        auto home = utils::getenv("HOME");
        if(!home.has_value()) {
            throw runtime_error("Could not determine config location - HOME environment variable not set!");
        }
        filesystem::path home_path = filesystem::path(home.value()) / ".config";
        return home_path;
    }
    return config_home.value();
}

filesystem::path
config::get_config_path(uint64_t network_id)
{
    auto config_home = config::get_config_home();

    stringstream node_path;
    node_path << utils::hex(network_id);

    return config_home / "ztproxy" / node_path.str();
}
