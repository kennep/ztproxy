#ifndef ZTPROXY_UTILS_H_INCLUDED
#define ZTPROXY_UTILS_H_INCLUDED

#include <string>
#include <vector>
#include <optional>
#include <system_error>
#include <atomic>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

namespace utils {
    void throw_errno();
    void throw_errno(const std::string &what);
    std::string gethostname();
    std::string inet_ntop(const sockaddr_in *addr);
    std::string inet_ntop(const sockaddr_in6 *addr);
    std::string inet_ntop(const sockaddr * addr);

    int get_port(sockaddr *addr);
    std::optional<std::string> getenv(const std::string &name);

    std::vector<std::string> split_string(const std::string &input, const std::string &delimiter, int max_elems = 0);

    std::string hex(uint64_t n);

    class cancellation_token
    {
    private:
        std::atomic<bool> cancel_flag;
    public:
        cancellation_token() : cancel_flag(false) { }
        void cancel() { cancel_flag.store(true); }
        bool is_cancelled() const { return cancel_flag.load(); }
    };
}

#endif