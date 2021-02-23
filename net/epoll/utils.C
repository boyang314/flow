#include "utils.H"
#include <string.h>
#include <sstream>
#include <iostream>

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
