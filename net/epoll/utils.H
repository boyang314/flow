#pragma once

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

struct SysUtil {
    static const char* getPeerIp(const struct sockaddr_in& addr);
    static std::string getReadableTcpAddress(const struct sockaddr_in& addr);

    static bool removeFromEpoll(int epoll, int fd);
};

