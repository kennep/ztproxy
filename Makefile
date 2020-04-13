.PHONY: all

SOURCES=ztproxy.cpp utils.cpp zt_manager.cpp proxy_manager.cpp config.cpp ip_address.cpp
HEADERS=utils.h multiplexer.h zt_manager.h proxy_manager.h config.h ip_address.h
CPPFLAGS=-Ilibzt/include
LIBZT_DEBUG=libzt/lib/debug/linux-x86_64/libzt.a
LIBZT_RELEASE=libzt/lib/release/linux-x86_64/libzt.a
LDFLAGS=-lpthread -static-libstdc++ -static-libgcc
all: ztproxy_debug ztproxy_release

ztproxy_debug: $(HEADERS) $(SOURCES) $(LIBZT_DEBUG)
	c++ $(CPPFLAGS) $(SOURCES) $(LIBZT_DEBUG) $(LDFLAGS) -std=c++17 -Wall -g -o ztproxy_debug

ztproxy_release: $(HEADERS) $(SOURCES) $(LIBZT_RELEASE)
	c++ $(CPPFLAGS) $(SOURCES) $(LIBZT_RELEASE) $(LDFLAGS) -std=c++17 -Wall -o ztproxy_release

libzt:
	git clone https://github.com/zerotier/libzt
	(cd libzt && make update && make patch)

$(LIBZT_DEBUG): libzt
	(cd libzt && make host_debug)

$(LIBZT_RELEASE): libzt
	(cd libzt && make host)

