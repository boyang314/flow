#include "utils.H"
#include "epollservice.H"

#include <string.h>
#include <sstream>
#include <iostream>
#include <unistd.h>

void SysUtil::split(const std::string str, char delim, std::vector<std::string>& tokens) {
    if (str.empty()) return;
    size_t last=0, next=0;
    while((next = str.find(delim, last)) != std::string::npos) {
        tokens.push_back(str.substr(last, next-last));
        last = next + 1;
    }
    tokens.push_back(str.substr(last));
}

bool SysUtil::parseTcpAddress(const std::string& addr, std::string& ip, int& port) {
    if (addr.find("tcp:") != 0) return false;
    std::vector<std::string> tokens;
    split(addr, '/', tokens);
    if (tokens.size() != 3) return false;
    if (tokens[0] != "tcp:") return false;
    ip = tokens[1];
    port = atoi(tokens[2].c_str());
    return true;
}

const char* SysUtil::getPeerIp(const struct sockaddr_in& addr) {
    return inet_ntoa(addr.sin_addr);
}

std::string SysUtil::getReadableTcpAddress(const struct sockaddr_in& addr) {
    char buf[64] = {0};
    std::ostringstream oss;
    oss << "tcp:/";
    if (inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf))) {
        oss << buf;
    } else {
        oss << "BadAddress";
    }
    oss << '/' << ntohs(addr.sin_port);
    return oss.str();
}

bool SysUtil::removeFromEpoll(int epoll, int fd) {
    if (epoll_ctl(epoll, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        std::cerr << "failed to remove fd:" << fd << " from epoll: " << epoll << '<' << errno << '|' << strerror(errno) << ">\n";
        return false;
    }
    return true;
}

bool SysUtil::registerToEpoll(int epoll, int fd, int events, EpollService* ptr) {
    struct epoll_event ev; ev.events = events; ev.data.ptr = ptr;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, fd, &ev) < 0) {
        std::cerr << "failed to add fd:" << fd << " to epoll: " << epoll << '<' << errno << '|' << strerror(errno) << ">\n";
        return false;
    }
    return true;
}

int SysUtil::createTcpClientFd(const std::string addr, int sendBufSize, int recvBufSize, int keepAliveTimeSec, int connectionTimeOutSec, struct sockaddr_in& remote) {
    std::string ip; int port=0;
    if (!parseTcpAddress(addr, ip, port)) {
        std::cerr << "failed to parse address:" << addr << std::endl;
        return -1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "failed to create socket fd" << std::endl;
        return -1;
    }

    //set fd nolinger
    //set sendBufSize
    //set recvBufSize
    //set tcpNoDelay
    //set tcpKeepAlive
    //set nonBlocking

    memset(&remote, 0, sizeof(remote));
    remote.sin_addr.s_addr = inet_addr(ip.c_str()); //???
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port); //must use htons
    int ret = ::connect(fd, (sockaddr*)&remote, sizeof(remote));
    if (ret < 0 && errno == EINPROGRESS) {
        std::cerr << "connect failed, try later EINPROGRESS" << std::endl;
        return -1;
    } else if (errno != 0) {
        std::cerr << "zz connect failed <" << errno << '|' << strerror(errno) << ">\n";
        return -1;
    }
    std::cout << "connected to " << addr << '@' << port << std::endl;
    return fd;
}

int SysUtil::createTcpServerFd(int port, int sendBufSize, int recvBufSize, struct sockaddr_in& local) {
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "Failed to create fd for TcpServer\n";
        return -1;
    }

    //set fd nolinger
    //set sendBufSize
    //set recvBufSize
    //set tcpNoDelay
    //set reuseAddress
    //set nonBlocking

    memset(&local, sizeof(local), 0);
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(port);
    if (bind(fd, (sockaddr*)&local, sizeof(local)) < 0) {
        std::cerr << "Failed to bind to port@" << port << std::endl;
        ::close(fd); return -1;
    }
    if (listen(fd, 8) < 0) {
        std::cerr << "Failed to listen port@" << port << std::endl;
        ::close(fd); return -1;
    }
    
    return fd;
}
