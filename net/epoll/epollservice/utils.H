#pragma once

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <vector>

class EpollService;

struct SysUtil {
    static bool setNonblocking(int fd, bool nonblocking);
    static bool setMcastTTL(int fd, uint8_t ttl);
    static bool setMcastLoopback(int fd, bool enabled);
    static bool setKeepalive(int fd, int timeoutSec);
    static bool setNoLinger(int fd);
    static bool setReuseAddr(int fd);
    static bool setNoDelay(int fd);

    static ssize_t getSendBufSize(int fd);
    static ssize_t getRecvBufSize(int fd);
    static bool setSendBufSize(int fd, int size);
    static bool setRecvBufSize(int fd, int size);
    static ssize_t sendQueueSize(int fd);
    static ssize_t recvQueueSize(int fd);

    static void split(const std::string str, char delim, std::vector<std::string>& tokens);
    static bool parseTcpAddress(const std::string& addr, std::string& ip, int& port);
    static bool parseUdpAddress(const std::string& addr, std::string& nic, std::string& ip, int& port);

    static const char* getPeerIp(const sockaddr_in& addr);
    static std::string getReadableTcpAddress(const sockaddr_in& addr);
    static bool getNicIp(int fd, const std::string& nic, std::string& nicip);
    static const char* getNicIp(const std::string& nic);

    static bool removeFromEpoll(int epoll, int fd);
    static bool registerToEpoll(int epoll, int fd, int events, EpollService* ptr);
    static bool modifyEpollFlag(int epoll, int fd, int events, EpollService* ptr);

    static int createTcpClientFd(const std::string& addr, sockaddr_in& remote);
    static int createTcpServerFd(int port, sockaddr_in& local);
    static int createUdpUnicastFd(const std::string& addr, sockaddr_in& remote);
    static int createMcastSenderFd(const std::string& addr, sockaddr_in& remote);
    static int createMcastReceiverFd(const std::string& addr, sockaddr_in& local);
};

