#include "utils.h"

#include <system_error>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

void
utils::throw_errno()
{
    throw system_error(errno, generic_category());
}

void
utils::throw_errno(const string &what)
{
    throw system_error(errno, generic_category(), what);
}

string
utils::gethostname()
{
    char buffer[HOST_NAME_MAX];

    if(::gethostname(buffer, HOST_NAME_MAX)!=0) {
        throw_errno();
    };
    return buffer;
}

utils::optional<string>
utils::getenv(const string &name)
{
    char *value = ::getenv(name.c_str());
    if(value == NULL) {
        return optional<string>();
    }
    return optional<string>(value);
}

vector<string>
utils::split_string(const string &input, const string &delimiter, int max_elems)
{
    vector<string> elems;
    size_t split_pos = 0;
    while(split_pos < input.size())
    {
        if(max_elems > 0 && elems.size() == (size_t)(max_elems - 1))
        {
            elems.emplace_back(input.substr(split_pos));
            break;
        }
        size_t delim_pos = input.find(delimiter, split_pos);
        if(delim_pos == string::npos)
        {
            elems.emplace_back(input.substr(split_pos));
            break;
        }
        elems.emplace_back(input.substr(split_pos, delim_pos - split_pos));
        split_pos = delim_pos + delimiter.size();
    }
    return elems;
}


string
utils::hex(uint64_t n)
{
    stringstream ss;
    ss << std::hex << n;
    return ss.str();
}


string
utils::inet_ntop(const sockaddr_in *addr)
{
    char buffer[255];
    if(::inet_ntop(AF_INET, &addr->sin_addr, buffer, 255)==NULL)
    {
        throw_errno("inet_ntop");
    }
    return buffer;
}

string
utils::inet_ntop(const sockaddr_in6 *addr)
{
    char buffer[255];
    if(::inet_ntop(AF_INET6, &addr->sin6_addr, buffer, 255)==NULL)
    {
        throw_errno("inet_ntop");
    }
    return buffer;
}

string
utils::inet_ntop(const sockaddr *addr)
{
    if(addr->sa_family == AF_INET)
    {
        return inet_ntop((sockaddr_in *) addr);
    }
    else if(addr->sa_family == AF_INET6)
    {
        return inet_ntop((sockaddr_in6 *)addr);
    }

    throw invalid_argument("Unsupported address family: " + to_string(addr->sa_family));
}

int
utils::get_port(sockaddr *addr)
{
    if(addr->sa_family == AF_INET)
    {
        return ::htons(((sockaddr_in *) addr)->sin_port);
    }
    else if(addr->sa_family == AF_INET6)
    {
        return ::htons(((sockaddr_in6 *) addr)->sin6_port);
    }

    throw invalid_argument("Unsupported address family: " + to_string(addr->sa_family));

}
