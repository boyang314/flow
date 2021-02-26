#pragma once

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <vector>

class EpollService;

struct SysUtil {
    static void split(const std::string str, char delim, std::vector<std::string>& tokens);
    static bool parseTcpAddress(const std::string& addr, std::string& ip, int& port);

    static const char* getPeerIp(const sockaddr_in& addr);
    static std::string getReadableTcpAddress(const sockaddr_in& addr);

    static bool removeFromEpoll(int epoll, int fd);
    static bool registerToEpoll(int epoll, int fd, int events, EpollService* ptr);

    static int createTcpClientFd(const std::string& addr, sockaddr_in& remote);
    static int createTcpServerFd(int port, sockaddr_in& local);
};

