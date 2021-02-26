#include "utils.H"
#include "epollservice.H"

#include <string.h>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <netdb.h>

bool SysUtil::setNonblocking(int fd, bool nonblocking) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    flags = nonblocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
}

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

const char* SysUtil::getPeerIp(const sockaddr_in& addr) {
    return inet_ntoa(addr.sin_addr);
}

std::string SysUtil::getReadableTcpAddress(const sockaddr_in& addr) {
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

int SysUtil::createTcpClientFd(const std::string& addr, sockaddr_in& remote) {
    std::string ip; int port=0;
    if (!parseTcpAddress(addr, ip, port)) {
        std::cerr << "failed to parse address:" << addr << std::endl;
        return -1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "failed to create socket fd for " << addr << std::endl;
        return -1;
    }

    //set fd nolinger
    //set sendBufSize //passin
    //set recvBufSize //passin
    //set tcpKeepAliveTimeSec //passin TimeSec
    //set tcpNoDelay
    //connectionTimeOutSec //passin TimeSec
    
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port); //must use htons
    struct hostent *he = gethostbyname(ip.c_str());
    if (!he) {
        std::cerr << "failed to resolve name " << ip << std::endl;
        return -1;
    }
    bcopy((char*)he->h_addr, (char*)&remote.sin_addr.s_addr, he->h_length);
    //memcpy(&remote.sin_addr, he->h_addr_list[0], he->h_length); //may not work correctly
    //remote.sin_addr.s_addr = inet_addr(ip.c_str()); //just dot notation, no alias

#if 0 //nonblocking connect & send
    SysUtil::setNonblocking(fd, true); //before 'connect', need to control timeout
    int ret = ::connect(fd, (sockaddr*)&remote, sizeof(remote));
    /*
    int count = 0;
    while(ret < 0 && errno == EINPROGRESS && ++count < 5) { //simple timeout and retry
        //poll based retry logic with connectionTimeOut
        poll(NULL,0,100); //100ms, better check fd ready to write with timeout
        int ret = ::connect(fd, (sockaddr*)&remote, sizeof(remote));
    }
    if (errno != EISCONN) {
        std::cerr << "connect failed to " << addr << '<' << errno << '|' << strerror(errno) << ">\n";
        ::close(fd); return -1;
    }
    */
    if (ret < 0 && errno == EINPROGRESS) { //using poll or select to monitor connection ready
        //poll based retry logic with connectionTimeOut
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLOUT | POLLERR | POLLHUP;
        int ret = poll(&pfd, 1, 500);
        if (ret > 0 && (pfd.revents & POLLOUT)) {
            return fd;
        } else {
            std::cerr << "connect failed to " << addr << '<' << errno << '|' << strerror(errno) << ">\n";
            ::close(fd); return -1;
        }
    }
#endif
//#if 0 //blocking connect & nonblocking send
    //tcp client blocking connect and nonblocking send
    int ret = ::connect(fd, (sockaddr*)&remote, sizeof(remote));
    if (ret != 0) {
        std::cerr << "connect failed to " << addr << ":" << strerror(errno) << std::endl;
        ::close(fd); return -1;
    }
    SysUtil::setNonblocking(fd, true); //after 'connect'
//#endif
    return fd;
}

int SysUtil::createTcpServerFd(int port, sockaddr_in& local) {
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "failed to create fd for TcpServer " << strerror(errno) << std::endl;
        return -1;
    }

    //set fd nolinger
    //set sendBufSize //passin para
    //set recvBufSize //passin para
    //set tcpNoDelay
    //set reuseAddress
    SysUtil::setNonblocking(fd, true);

    memset(&local, sizeof(local), 0);
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(port);
    if (bind(fd, (sockaddr*)&local, sizeof(local)) < 0) {
        std::cerr << "failed to bind to port@" << port << " " << strerror(errno) << std::endl;
        ::close(fd); return -1;
    }
    if (listen(fd, 8) < 0) { //8 backlog
        std::cerr << "failed to listen at port@" << port << " " << strerror(errno) << std::endl;
        ::close(fd); return -1;
    }
    
    return fd;
}
