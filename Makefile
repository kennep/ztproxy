.PHONY: all

SOURCES=ztproxy.cpp utils.cpp zt_manager.cpp proxy_manager.cpp config.cpp ip_address.cpp version.cpp
HEADERS=utils.h zt_manager.h proxy_manager.h config.h ip_address.h version.h
CPPFLAGS=-Ilibzt/include
LIBZT_DEBUG=libzt/lib/debug/linux-x86_64/libzt.a
LIBZT_RELEASE=libzt/lib/release/linux-x86_64/libzt.a
LDFLAGS=-lpthread -static-libstdc++ -static-libgcc
CXX=c++
all: ztproxy_debug ztproxy_release

ztproxy_debug: $(HEADERS) $(SOURCES)
	$(CXX) $(CPPFLAGS) $(SOURCES) $(LIBZT_DEBUG) $(LDFLAGS) -std=c++11 -Wall -g -o ztproxy_debug

ztproxy_release: $(HEADERS) $(SOURCES)
	$(CXX) $(CPPFLAGS) $(SOURCES) $(LIBZT_RELEASE) $(LDFLAGS) -std=c++11 -Wall -o ztproxy_release


