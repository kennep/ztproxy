SOURCES=ztproxy.cpp utils.cpp zt_manager.cpp proxy_manager.cpp config.cpp ip_address.cpp
HEADERS=utils.h multiplexer.h zt_manager.h proxy_manager.h config.h ip_address.h
CFLAGS=-Ilibzt/include
LIBZT=libzt/lib/debug/linux-x86_64/libzt.a
LDFLAGS=-lpthread -static-libstdc++ -static-libgcc
CXX=c++
ztproxy: $(HEADERS) $(SOURCES) $(LIBZT_DEBUG)
	$(CXX) $(CPPFLAGS) $(SOURCES) $(LIBZT_DEBUG) $(LDFLAGS) -std=c++11 -Wall -g -o ztproxy_debug

