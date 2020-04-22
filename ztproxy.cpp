#include "utils.h"
#include "config.h"
#include "proxy_manager.h"
#include "zt_manager.h"
#include "command_server.h"
#include "version.h"


#include <iostream>
#include <string>

#include <unistd.h>
#include <getopt.h>

using namespace std;
using namespace ztproxy;

void
usage(string program)
{
  cerr << "Usage: " << program << " --network-id NETWORKID [--node-name NODENAME] [--ipv4] [--ipv6] SRCPORT:TARGETHOST:TARGETPORT..." << endl
    << "Options:" << endl
    << "  --network-id|-n     Join ZeroTier network NETWORKID. Required." << endl
    << "  --ipv4|-4           Use IPv4 only to connect to remote targets (default)" << endl
    << "  --ipv6|-6           Use IPv6 only to connect to remote targets" << endl
    << "  --help|-h           Print this help" << endl
    << "  --version|-v        Print version number" << endl
	  << "Arguments:" << endl
	  << "   SRCPORT            Source port on local machine to listen on" << endl
	  << "   TARGETHOST         Target host to forward traffic to on the ZeroTier network" << endl
	  << "   TARGETPORT         Target port to forward traffic to on the ZeroTier network" << endl
    ;
}

int
main(int argc, char **argv)
{
  uint64_t network_id = 0;
  char *tmp;
  char c;
  bool use_ipv4 = false;
  bool use_ipv6 = false;
  static struct option long_options[] = {
            {"network-id",  required_argument, 0,  'n' },
            {"help",        no_argument,       0,  'h' },
            {"ipv4",        no_argument,       0,  '4' },
            {"ipv6",        no_argument,       0,  '6' },
            {"version",     no_argument,       0,  'v' },
            {0,             0,                 0,  0 }
        };

  opterr = 1;

  while ((c = getopt_long(argc, argv, "n:h46v", long_options, NULL)) != -1)
    switch(c)
      {
      case 'n':
        network_id = strtoul(optarg, &tmp, 16);
        if(network_id == 0 || *tmp != 0) {
          fprintf(stderr, "Error: --network-id must be a hexadecimal number\n");
          usage(argv[0]);
          return EXIT_FAILURE;
        }
        break;
      case '4':
        use_ipv4 = true;
        break;
      case '6':
        use_ipv6 = true;
        break;
      case 'h':
        usage(argv[0]);
        return EXIT_SUCCESS;
      case '?':
        usage(argv[0]);
        return EXIT_FAILURE;
      case 'v':
        cerr << argv[0] << " version " << ztproxy::version << endl;
        return EXIT_SUCCESS;
      default:
        abort ();
      }

  if(use_ipv4 == false && use_ipv6 == false)
  {
    use_ipv4 = true;
  }

  if(network_id == 0)
  {
	  cerr << "Error: --network-id argument must be specified." << endl;
	  usage(argv[0]);
	  return EXIT_FAILURE;
  }
  cerr << "Network ID: " << utils::hex(network_id) << endl;

  try {
    zt_manager ztm(network_id);
    ztm.start();
    proxy_manager mgr(use_ipv4, use_ipv6);  

    for(int i=optind; i<argc; ++i)
    {
      mgr.add_proxy(argv[i]);
    }

    command_server cmd_server(network_id, mgr);
    cmd_server.run();

    std::string bar;
    cin >> bar;

    return 0;
  }
  catch(const exception &e)
  {
    cerr << "ERROR: " << e.what() << endl;
    return 1;
  }

}
