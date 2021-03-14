#include "utils.H"
#include "epollservice.H"

#include <string.h>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/tcp.h> //keepalive option
#include <linux/sockios.h> //SIOCINQ, SIOCOUTQ
#include <poll.h>
#include <netdb.h>

bool SysUtil::setNonblocking(int fd, bool nonblocking) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    flags = nonblocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
}

bool SysUtil::setMcastTTL(int fd, uint8_t ttl) {
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&ttl, sizeof(ttl)) < 0) {
        std::cerr << "failed to set multicast_ttl fd " << fd << std::endl;
        return false;
    }
    return true;
}

bool SysUtil::setMcastLoopback(int fd, bool enabled) {
    char loopback = (char)enabled;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, (void*)&loopback, sizeof(loopback)) < 0) {
        std::cerr << "failed to set multicast loopback fd " << fd << std::endl;
        return false;
    }
    return true;
}

bool SysUtil::setKeepalive(int fd, int timeoutSec) {
    if (0 == timeoutSec) return true;
    //allow connection idle for <timeoutSec>
    //use up to 3 tcp probe to check whether connection is intact
    //should complete with 10% of <timeoutSec>
    int enabled = 1;
    int probeCount = 3;
    int probeInterval = (timeoutSec / (probeCount * 10)) + 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enabled, sizeof(enabled)) < 0) {
        std::cerr << "failed to set SO_KEEPALIVE fd " << fd << std::endl;
        return false;
    }
    if (setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &timeoutSec, sizeof(timeoutSec)) < 0) {
        std::cerr << "failed to set TCP_KEEPIDLE fd " << fd << std::endl;
        return false;
    }
    if (setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &probeCount, sizeof(probeCount)) < 0) {
        std::cerr << "failed to set TCP_KEEPCNT fd " << fd << std::endl;
        return false;
    }
    if (setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &probeInterval, sizeof(probeInterval)) < 0) {
        std::cerr << "failed to set TCP_KEEPINTVL fd " << fd << std::endl;
        return false;
    }
    return true;
}

bool SysUtil::setNoLinger(int fd) {
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger)) < 0) {
        std::cerr << "failed to set SO_LINGER fd " << fd << std::endl;
        return false;
    }
    return true;
}

bool SysUtil::setReuseAddr(int fd) {
    int flag = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag)) < 0) {
        std::cerr << "failed to set SO_REUSEADDR fd " << fd << std::endl;
        return false;
    }
    return true;
}

bool SysUtil::setNoDelay(int fd) {
    int flag = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)) < 0) {
        std::cerr << "failed to set TCP_NODELAY fd " << fd << std::endl;
        return false;
    }
    return true;
}

ssize_t SysUtil::getSendBufSize(int fd) {
    ssize_t bufSize = 0;
    socklen_t len = sizeof(bufSize);
    if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufSize, &len) < 0) {
        std::cerr << "failed to getsockopt SO_SNDBUF " << strerror(errno) << std::endl;
        return -1;
    }
    return bufSize;
}

ssize_t SysUtil::getRecvBufSize(int fd) {
    ssize_t bufSize = 0;
    socklen_t len = sizeof(bufSize);
    if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufSize, &len) < 0) {
        std::cerr << "failed to getsockopt SO_RCVBUF " << strerror(errno) << std::endl;
        return -1;
    }
    return bufSize;
}

bool SysUtil::setSendBufSize(int fd, int size) {
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) < 0) {
        std::cerr << "failed to setsockopt SO_SNDBUF " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool SysUtil::setRecvBufSize(int fd, int size) {
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) < 0) {
        std::cerr << "failed to setsockopt SO_RCVBUF " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

ssize_t SysUtil::sendQueueSize(int fd) {
    if (0 == fd) return -1;
    int value = 0;
    int error = ioctl(fd, SIOCOUTQ, &value);
    return error ? -1 : value;
}

ssize_t SysUtil::recvQueueSize(int fd) {
    if (0 == fd) return -1;
    int value = 0;
    int error = ioctl(fd, SIOCINQ, &value);
    return error ? -1 : value;
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

bool SysUtil::parseUdpAddress(const std::string& addr, std::string& nic, std::string& ip, int& port) {
    if (addr.find("udp:") != 0) return false;
    std::vector<std::string> tokens;
    split(addr, '/', tokens);
    if (tokens.size() != 4) return false;
    if (tokens[0] != "udp:") return false;
    nic = tokens[1];
    ip = tokens[2];
    port = atoi(tokens[3].c_str());
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

bool SysUtil::getNicIp(int fd, const std::string& nic, std::string& nicip) {
    struct ifreq ifc;
    strcpy(ifc.ifr_name, nic.c_str());
    if (ioctl(fd, SIOCGIFADDR, &ifc) < 0) {
        std::cerr << "failed to get nic ip " << nic << ' ' << strerror(errno) << std::endl;
        return false;
    }
    struct sockaddr_in* interface = (sockaddr_in*)&ifc.ifr_addr;
    nicip = inet_ntoa(interface->sin_addr);
    return true;
}

const char* SysUtil::getNicIp(const std::string& nic) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct ifreq ifc;
    strcpy(ifc.ifr_name, nic.c_str());
    if (ioctl(fd, SIOCGIFADDR, &ifc) < 0) {
        std::cerr << "failed to get nic ip " << nic << ' ' << strerror(errno) << std::endl;
        return nullptr;
    }
    struct sockaddr_in* interface = (sockaddr_in*)&ifc.ifr_addr;
    ::close(fd);
    return inet_ntoa(interface->sin_addr);
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

bool SysUtil::modifyEpollFlag(int epoll, int fd, int events, EpollService* ptr) {
    struct epoll_event ev; ev.events = events; ev.data.ptr = ptr;
    if (epoll_ctl(epoll, EPOLL_CTL_MOD, fd, &ev) < 0) {
        std::cerr << "failed to modify fd:" << fd << " to epoll: " << epoll << '<' << errno << '|' << strerror(errno) << ">\n";
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

    setNoLinger(fd);
    std::cout << "sendbufsize:" << getSendBufSize(fd) << " recvbufsize:" << getRecvBufSize(fd) << std::endl;
    //setSendBufSize(fd, 1024);
    //setRecvBufSize(fd, 1024);
    //std::cout << "sendbufsize:" << getSendBufSize(fd) << " recvbufsize:" << getRecvBufSize(fd) << std::endl;
    setKeepalive(fd, 60); //timeoutSec=60
    setNoDelay(fd);
    
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

    //nonblocking connect & send
    SysUtil::setNonblocking(fd, true); //before 'connect', need to control timeout
    int ret = ::connect(fd, (sockaddr*)&remote, sizeof(remote));
#if 0
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
#endif
    if (ret < 0 && errno == EINPROGRESS) { //using poll or select to monitor connection ready
        //poll based retry logic with connectionTimeOut
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLOUT | POLLERR | POLLHUP;
        //connectionTimeOutSec 
        int ret = poll(&pfd, 1, 500); //size=1;timeoutSec=500ms
        if (ret > 0 && (pfd.revents & POLLOUT)) {
            return fd;
        } else {
            std::cerr << "connect failed to " << addr << '<' << errno << '|' << strerror(errno) << ">\n";
            ::close(fd); return -1;
        }
    }

#if 0 
    //blocking connect & nonblocking send
    //tcp client blocking connect and nonblocking send
    int ret = ::connect(fd, (sockaddr*)&remote, sizeof(remote));
    if (ret != 0) {
        std::cerr << "connect failed to " << addr << ":" << strerror(errno) << std::endl;
        ::close(fd); return -1;
    }
    SysUtil::setNonblocking(fd, true); //after 'connect'
#endif
    return fd;
}

int SysUtil::createTcpServerFd(int port, sockaddr_in& local) {
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "failed to create fd for TcpServer " << strerror(errno) << std::endl;
        return -1;
    }

    setNoLinger(fd);
    std::cout << "sendbufsize:" << getSendBufSize(fd) << " recvbufsize:" << getRecvBufSize(fd) << std::endl;
    //setSendBufSize(fd, 1024);
    //setRecvBufSize(fd, 1024);
    //std::cout << "sendbufsize:" << getSendBufSize(fd) << " recvbufsize:" << getRecvBufSize(fd) << std::endl;
    setNoDelay(fd);
    setReuseAddr(fd);
    setNonblocking(fd, true);

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

int SysUtil::createUdpUnicastFd(const std::string& addr, sockaddr_in& remote) {
    std::string nic, ip; int port=0;
    if (!parseUdpAddress(addr, nic, ip, port)) {
        std::cerr << "failed to parse udp address:" << addr << std::endl;
        return -1;
    }

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        std::cerr << "failed to create udp socket fd for " << addr << std::endl;
        return -1;
    }

    if (!SysUtil::setNonblocking(fd, true)) {
        std::cerr << "failed to set nonblocking to " << addr << std::endl;
        ::close(fd); return -1;
    }

    setReuseAddr(fd);

    sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(port);
    if (::bind(fd, (sockaddr*)&local, sizeof(local)) < 0) {
        std::cerr << "failed to bind fd " << addr << " " << strerror(errno) << std::endl;
        ::close(fd); return -1;
    }

    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port);
    struct hostent* server = gethostbyname(ip.c_str());
    if (!server) {
        std::cerr << "failed to gethostbyname " << ip << std::endl;
        ::close(fd); return -1;
    }
    bcopy((char*)server->h_addr, (char*)&remote.sin_addr.s_addr, server->h_length);

    return fd;
}

int SysUtil::createMcastSenderFd(const std::string& addr, sockaddr_in& remote) {
    std::string nic, ip; int port=0;
    if (!parseUdpAddress(addr, nic, ip, port)) {
        std::cerr << "failed to parse udp address:" << addr << std::endl;
        return -1;
    }

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        std::cerr << "failed to create udp socket fd for " << addr << std::endl;
        return -1;
    }

    if (!SysUtil::setNonblocking(fd, true)) {
        std::cerr << "failed to set nonblocking to " << addr << std::endl;
        ::close(fd); return -1;
    }

    //setSendBufSize(fd, 1024);
    std::cout << "mcast sendBufSize:" << getSendBufSize(fd) << std::endl;

    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = inet_addr(ip.c_str());
    remote.sin_port = htons(port);

    setMcastTTL(fd, 7);
    setMcastLoopback(fd, true);

    std::string nicip;
    if (!getNicIp(fd, nic, nicip)) {
        std::cerr << "failed to get nicip for " << nic << std::endl;
        ::close(fd); return -1;
    }
    struct in_addr iface;
    iface.s_addr = inet_addr(nicip.c_str());
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, (char*)&iface, sizeof(iface)) < 0) {
        std::cerr << "failed to set multicast interface " << addr << " " << strerror(errno) << std::endl;
        ::close(fd); return -1;
    }
    return fd;
}

int SysUtil::createMcastReceiverFd(const std::string& addr, sockaddr_in& local) {
    std::string nic, ip; int port=0;
    if (!parseUdpAddress(addr, nic, ip, port)) {
        std::cerr << "failed to parse udp address:" << addr << std::endl;
        return -1;
    }

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        std::cerr << "failed to create udp socket fd for " << addr << std::endl;
        return -1;
    }

    if (!setNonblocking(fd, true)) {
        std::cerr << "failed to set nonblocking to " << addr << std::endl;
        ::close(fd); return -1;
    }

    setReuseAddr(fd);
    //setRecvBufSize(fd, 1024);
    std::cout << "mcast recvBufSize:" << getRecvBufSize(fd) << std::endl;

    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = inet_addr(ip.c_str());
    local.sin_port = htons(port);
    if (::bind(fd, (sockaddr*)&local, sizeof(local)) < 0) {
        std::cerr << "failed to bind fd " << addr << " " << strerror(errno) << std::endl;
        ::close(fd); return -1;
    }

    struct ifreq ifc;
    strcpy(ifc.ifr_name, nic.c_str());
    if (ioctl(fd, SIOCGIFADDR, &ifc) < 0) {
        std::cerr << "failed to get nic ip " << nic << ' ' << strerror(errno) << std::endl;
        return false;
    }
    struct sockaddr_in* interface = (sockaddr_in*)&ifc.ifr_addr;
    struct ip_mreq group; memset(&group, 0, sizeof(group));
    group.imr_multiaddr.s_addr = inet_addr(ip.c_str());
    group.imr_interface.s_addr = inet_addr(inet_ntoa(interface->sin_addr));
    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&group, sizeof(group)) < 0) {
        std::cerr << "failed to set multicast group " << addr << " " << strerror(errno) << std::endl;
        ::close(fd); return -1;
    }
    return fd;
}

