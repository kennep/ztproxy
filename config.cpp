#include "config.h"
#include "utils.h"

#include <stdexcept>
#include <sstream>
#include <iomanip>

using namespace std;

utils::path
config::get_config_home()
{
    auto config_home = utils::getenv("XDG_CONFIG_HOME");
    if(!config_home.has_value())
    {
        auto home = utils::getenv("HOME");
        if(!home.has_value()) {
            throw runtime_error("Could not determine config location - HOME environment variable not set!");
        }
        utils::path home_path = utils::path(home.value()) / ".config";
        return home_path;
    }
    return config_home.value();
}

utils::path
config::get_config_path(uint64_t network_id)
{
    auto config_home = config::get_config_home();

    stringstream node_path;
    node_path << utils::hex(network_id);

    return config_home / "ztproxy" / node_path.str();
}
