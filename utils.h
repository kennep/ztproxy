#ifndef ZTPROXY_UTILS_H_INCLUDED
#define ZTPROXY_UTILS_H_INCLUDED

#include <string>
#include <vector>
#include <optional>
#include <system_error>
#include <atomic>
#include <iostream>

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

    template<typename T>
    class optional
    {
    private:
        T val;
        bool present;
    public:
        optional() : val(), present(false) {}
        optional(const T &v) : val(v), present(true) {}
        bool has_value() { return present; }
        T& value() { return val; }
    };

    class path
    {
    private:
        std::string val;
    public:
        path(const std::string &v) : val(v) {}
        path(const char *v) : val(v) {}
        const char *c_str() { return val.c_str(); }
        path operator / (const path &elem) const
        {
            return path(val + "/" + elem.val);
        }
        operator std::string() const { return val; }
        friend std::ostream& operator<<(std::ostream& os, const path& p);
    };

    inline std::ostream& operator<<(std::ostream& os, const path& p)
    {
        os << p.val;
        return os;
    }

    int get_port(sockaddr *addr);
    optional<std::string> getenv(const std::string &name);

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