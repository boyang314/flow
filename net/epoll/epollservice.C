#include "epollservice.H"
#include "utils.H"

#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cassert>

//TcpConnection

TcpConnection::TcpConnection(int epoll, const std::string& peerAddr, StreamListener* listener) 
: EpollService(epoll), addr_(peerAddr), mode_('O'), listener_(listener) {
    assert(epoll != 0);
    assert(listener != nullptr);
    memset(&local_, 0, sizeof(local_));
    memset(&remote_, 0, sizeof(remote_));
}

TcpConnection::TcpConnection(int epoll, int fd, const sockaddr_in& remote, StreamListener* listener) 
: EpollService(epoll), addr_(SysUtil::getReadableTcpAddress(remote)), mode_('I'), fd_(fd), listener_(listener) {
    assert(epoll != 0);
    assert(listener != nullptr);
    memset(&local_, 0, sizeof(local_));
    memcpy(&remote_, &remote, sizeof(remote));
}

TcpConnection::~TcpConnection() {
    if (0 != fd_) {
        SysUtil::removeFromEpoll(epoll_, fd_);
        ::close(fd_);
    }
}

bool TcpConnection::isOpen() const {
    return 0 < fd_;
}

void TcpConnection::close() {
    if (0 != fd_) {
        //trigger onEpollError
        closing_ = true;
        ::shutdown(fd_, SHUT_RDWR);
    }
}

bool TcpConnection::initialize() {
    closing_ = false;
    if (0 == fd_) {
        //tcpSendBufSize, tcpRecvBufSize, tcpKeepAliveTimeSec, connectTimeOutSec/1
        fd_ = SysUtil::createTcpClientFd(addr_, remote_);
        if (0 > fd_) {
            std::cerr << "failed to create tcp client fd to " << addr_ << std::endl;
            fd_ = 0; return false;
        }
    }
    //resovle none dot names
    socklen_t len = sizeof(local_);
    if (getsockname(fd_, (sockaddr*)&local_, &len) < 0) {
        std::cerr << "failed getsockname for " << addr_ << " " << strerror(errno) << std::endl;
        ::close(fd_); fd_ = 0; return false;
    }
    laddr_ = SysUtil::getReadableTcpAddress(local_);
    if (!SysUtil::registerToEpoll(epoll_, fd_, EPOLLIN|EPOLLERR|EPOLLHUP|EPOLLRDHUP, static_cast<EpollService*>(this))) {
        std::cerr << "TcpConn::init failed to register to epoll for " << addr_ << std::endl; 
        ::close(fd_); fd_ = 0; return false;
    }
    listener_->onConnect(this);
    std::cout << "socket:" << *this << std::endl;
    return true;
}

void TcpConnection::onEpollIn() {
    const unsigned max_msg_size = 1024;
    char buf[max_msg_size];
    size_t sizeProcessed = 0;
    while(sizeProcessed < 100 * max_msg_size) { //for nonblocking multiple recv until -1 returned
        if (0 == fd_) return; //socket closed by listener or not properly removed from epoll
        if (closing_) { onEpollError(); return; }
        //throtling required?
        int ret = recv(fd_, buf, sizeof(buf), 0);
        std::cout << fd_ << ":recv:" << ret << std::endl;
        switch(ret) {
            case 0:
                onEpollError(); //peer initiate shutdown
                return;
            case -1:
                if (EAGAIN != errno && EWOULDBLOCK != errno) onEpollError(); //unexpected
                return;
            default:
                listener_->onPacket(buf, ret, remote_, this);
                sizeProcessed += ret;
                std::cout << fd_ << ":sizeProcessed:" << sizeProcessed << std::endl;
                //return; //for blocking
                break; //for nonblocking
        }
    }
}

void TcpConnection::onEpollError() {
    int fd = fd_;
    fd_ = 0;
    if (0 < fd) {
        SysUtil::removeFromEpoll(epoll_, fd);
        ::close(fd);
        std::cout << "closed connection (" << this << ") unregister from epoll " << epoll_ << std::endl;
        if (listener_) listener_->onDisconnect(this);
    }
}

ssize_t TcpConnection::send(const char* msg, size_t len) {
    if (0 == len) return 0;
    ssize_t bytesSent = ::send(fd_, msg, len, MSG_NOSIGNAL);
    if (0 > bytesSent) {
        if (EWOULDBLOCK != errno && EAGAIN != errno) 
            std::cerr << "send unexpected <" << errno << '|' << strerror(errno) << "> to " << addr_ << std::endl;
    }
    return bytesSent;
}

std::ostream& operator<<(std::ostream& os, const TcpConnection& obj) {
    os << '<' << obj.mode_ << '|' << obj.fd_ << '@' << &obj << '|' << obj.laddr_ << '|' << obj.addr_ << '>';
}

//TcpConnectionServer

TcpConnectionServer::TcpConnectionServer(int epoll, int port, StreamListener* handler) 
: EpollService(epoll), port_(port), handler_(handler) {
    assert(epoll != 0);
    assert(handler != nullptr);
    //memset(&local_, 0, sizeof(local_)); //createServerFd will initizlize it
}

TcpConnectionServer::~TcpConnectionServer() {
    close();
}

TcpConnection* TcpConnectionServer::accept(StreamListener* listener) {
    assert(listener != nullptr);
    sockaddr_in remote;
    socklen_t len = sizeof(remote);
    int fd = ::accept(fd_, (sockaddr*)&remote, &len);
    if (fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) std::cerr << "TcpServer@" << port_ << " failed to accept connection\n";
        return nullptr;
    }

    //getnameinfo of remote host
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    if (getnameinfo((sockaddr*)&remote, len, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
        std::cout << "TcpServer@" << port_ << " accepted connection from host:" << hbuf << ":port:" << sbuf << ":fd:" << fd << std::endl;
    } else {
        std::cerr << "failed to getnameinfo " << strerror(errno) << std::endl;
        ::close(fd); return nullptr;
    }

    SysUtil::setNonblocking(fd, true);
    //setSendBufSize
    //setRecvBufSize
    std::cout << "accept sendBufSize:" << SysUtil::getSendBufSize(fd) << " recvBufSize:" << SysUtil::getRecvBufSize(fd) << " sendQSize: " << SysUtil::sendQueueSize(fd) << " recvQSize:" << SysUtil::recvQueueSize(fd) << std::endl;
    SysUtil::setKeepalive(fd, 60); //timeoutSec=60

    return createServerConnection(epoll_, fd, remote, listener);
}

TcpConnection* TcpConnectionServer::createServerConnection(int epoll, int fd, const sockaddr_in& remote, StreamListener* listener) {
    TcpConnection* newConn = new TcpConnection(epoll, fd, remote, listener);
    if (!newConn->initialize()) {
        std::cerr << "failed to initialize connection " << SysUtil::getReadableTcpAddress(remote) << std::endl;
        delete newConn; return nullptr;
    }
    return newConn;
}

void TcpConnectionServer::close() {
    std::cout << "TcpServer remove from epoll " << epoll_ << " close fd " << fd_ << std::endl;
    if (0 < fd_) {
        SysUtil::removeFromEpoll(epoll_, fd_);
        ::close(fd_); fd_ = 0;
    }
}

void TcpConnectionServer::refuse() {
    sockaddr_in remote;
    socklen_t len = sizeof(remote);
    int fd = ::accept(fd_, (sockaddr*)&remote, &len);
    if (0 <= fd) ::close(fd);
}

bool TcpConnectionServer::initialize() {
    fd_ = SysUtil::createTcpServerFd(port_, local_); //local_ memset to 0 in ctor
    if (fd_ < 0) {
        std::cerr << "Failed to create server fd port:" << port_ << std::endl;
        return false;
    }
    if (!SysUtil::registerToEpoll(epoll_, fd_, EPOLLIN|EPOLLERR|EPOLLHUP, static_cast<EpollService*>(this))) {
        std::cerr << "Failed to register to epoll " << epoll_ << " fd " << fd_ << std::endl;
        ::close(fd_); fd_ = 0; return false;
    }
    std::cout << "TcpSever:epoll:" << epoll_ << ":fd:" << fd_ << ':' << SysUtil::getReadableTcpAddress(local_) << std::endl;
    return true;
}

void TcpConnectionServer::onEpollIn() {
    accept(handler_);
}

void TcpConnectionServer::onEpollError() {
    close();
}

//UdpUnicast

UdpUnicast::UdpUnicast(int epoll, const std::string& addr)
: EpollService(epoll), addr_(addr) {
}

UdpUnicast::~UdpUnicast() {
    close();
}

void UdpUnicast::close() {
    if (fd_ > 0) {
        SysUtil::removeFromEpoll(epoll_, fd_);
        ::close(fd_); fd_ = 0;
        std::cout << "close UdpUnicast (" << this << ") and remove from epoll: " << epoll_ << std::endl;
    }
}

ssize_t UdpUnicast::send(const char* msg, size_t len) {
    if (0 == len) return 0;
    return ::sendto(fd_, msg, len, 0, (sockaddr*)&remote_, sizeof(remote_));
}

bool UdpUnicast::addListener(DatagramListener* listener) {
    if (listener) {
        listeners_.push_back(listener);
        return true;
    }
    return false;
}

bool UdpUnicast::initialize() {
    fd_ = SysUtil::createUdpUnicastFd(addr_, remote_);
    if (fd_ < 0) {
        std::cerr << "failed to create unicastfd for " << addr_ << std::endl;
        return false;
    }
    if (!SysUtil::registerToEpoll(epoll_, fd_, EPOLLIN|EPOLLET|EPOLLERR|EPOLLHUP, static_cast<EpollService*>(this))) {
        std::cerr << "failed to register epoll events for " << addr_ << std::endl;
        ::close(fd_); fd_ = 0; return false;
    }
    return true;
}

void UdpUnicast::onEpollIn() {
    char buf[1024];
    struct sockaddr_in remote;
    socklen_t len = sizeof(remote);
    while(true) {
        int ret = ::recvfrom(fd_, buf, sizeof(buf), 0, (sockaddr*)&remote, &len);
        if (0 == ret) {
            static uint64_t counter = 0;
            if (++counter % 1000000L == 1) 
                std::cerr << "recvfrom got " << counter << " zero bytes packet" << std::endl;
        } else if (ret > 0) {
            std::cout << fd_ << ":recvfrom:" << ret << ':' << buf << std::endl;
            for(auto& listener : listeners_) listener->onPacket(buf, ret, remote);
        } else break;
    }
}

void UdpUnicast::onEpollError() {
    close();
}

//McastSender

McastSender::McastSender(int epoll, const std::string& addr)
:EpollService(epoll), addr_(addr) {}

McastSender::~McastSender() { close(); }

void McastSender::close() {
    if (fd_ > 0) {
        //sender not register with epoll
        ::close(fd_); fd_ = 0;
        std::cout << "close multicast sender (" << this << ")" << epoll_ << std::endl;
    }
}

ssize_t McastSender::send(const char* msg, size_t len) {
    if (0 == len) return 0;
    return ::sendto(fd_, msg, len, 0, (sockaddr*)&remote_, sizeof(remote_));
}

bool McastSender::initialize() {
    fd_ = SysUtil::createMcastSenderFd(addr_, remote_);
    if (fd_ < 0) {
        std::cerr << "failed to create mcastsender fd for " << addr_ << std::endl;
        return false;
    }
    return true;
}

void McastSender::onEpollIn() {}

void McastSender::onEpollError() { close(); }

//McastReceiver

McastReceiver::McastReceiver(int epoll, const std::string& addr)
:EpollService(epoll), addr_(addr) {}

McastReceiver::~McastReceiver() { close(); }

void McastReceiver::close() {
    if (fd_ > 0) {
        SysUtil::removeFromEpoll(epoll_, fd_);
        ::close(fd_); fd_ = 0;
        std::cout << "close multicast receiver (" << this << ") and remove from epoll: " << epoll_ << std::endl;
    }
}

bool McastReceiver::initialize() {
    fd_ = SysUtil::createMcastReceiverFd(addr_, local_);
    if (fd_ < 0) {
        std::cerr << "failed to create mcastreceiver fd for " << addr_ << std::endl;
        return false;
    }
    if (!SysUtil::registerToEpoll(epoll_, fd_, EPOLLIN|EPOLLET|EPOLLERR|EPOLLHUP, static_cast<EpollService*>(this))) {
        std::cerr << "failed to register epoll events for " << addr_ << std::endl;
        ::close(fd_); fd_ = 0; return false;
    }
    return true;
}

void McastReceiver::onEpollIn() {
    char buf[1024];
    struct sockaddr_in remote;
    socklen_t len = sizeof(remote);
    while(true) {
        int ret = ::recvfrom(fd_, buf, sizeof(buf), 0, (sockaddr*)&remote, &len);
        if (0 == ret) {
            static uint64_t counter = 0;
            if (++counter % 1000000L == 1) 
                std::cerr << "mcast::recvfrom got " << counter << " zero bytes packet" << std::endl;
        } else if (ret > 0) {
            std::cout << fd_ << ":mcast::recvfrom:" << ret << ':' << buf << std::endl;
            //for(auto& listener : listeners_) listener->onPacket(buf, ret, remote);
        } else break;
    }
}

void McastReceiver::onEpollError() { close(); }

//EpollActiveObject

EpollActiveObject::EpollActiveObject(const std::string& name) : name_(name) {
    epoll_ = epoll_create(max_epoll_events_);
    assert(epoll_ > 0);
    std::cout << *this << " initialized\n";
}

TcpConnection* EpollActiveObject::createTcpConnection(const std::string& peerAddr, StreamListener* listener) {
    TcpConnection* conn = new TcpConnection(epoll_, peerAddr, listener);
    if (!conn->initialize()) { delete conn; return nullptr; }
    return conn;
}

TcpConnectionServer* EpollActiveObject::createTcpConnectionServer(unsigned port, StreamListener* handler) {
    TcpConnectionServer* server = new TcpConnectionServer(epoll_, port, handler);
    if (!server->initialize()) { delete server; return nullptr; }
    return server;
}

UdpUnicast* EpollActiveObject::createUdpUnicast(const std::string& addr) {
    UdpUnicast* conn = new UdpUnicast(epoll_, addr);
    if (!conn->initialize()) { delete conn; return nullptr; }
    return conn;
}

McastSender* EpollActiveObject::createMcastSender(const std::string& addr) {
    McastSender* conn = new McastSender(epoll_, addr);
    if (!conn->initialize()) { delete conn; return nullptr; }
    return conn;
}

McastReceiver* EpollActiveObject::createMcastReceiver(const std::string& addr) {
    McastReceiver* conn = new McastReceiver(epoll_, addr);
    if (!conn->initialize()) { delete conn; return nullptr; }
    return conn;
}

void EpollActiveObject::start() {
    if (running_) return;
    running_ = 1;
    thread_ = std::thread{[this](){ this->run();}};
}

void EpollActiveObject::stop() {
    if (running_) {
        running_ = 0;
        thread_.join();
    }
}

void EpollActiveObject::run() {
    std::cout << *this << " started\n";
    //pin this thread to core and set realtime if possible
    int counter = 0;
    while(running_) { //unlikely thread interrupted
        //check unlikely system condition for exit
        struct epoll_event events[max_epoll_events_];
        int timeoutInMs = 1000; //read from config or calculate the right value
        int n = epoll_wait(epoll_, events, sizeof(events), timeoutInMs);
        ++counter;
        if (n < 0) { //unlikely
            if (errno == EINTR) continue;
            std::cerr << " epoll_wait error<" << errno << '|' << strerror(errno) << ">\n";
            break;
        } else if (n == 0) {
            //std::cout << " timeout...\n";
            //if (counter >= 10) break;
            continue;
        }

        for(int i=0; i<n; ++i) {
            //process epoll event
            struct epoll_event& event = events[i];
            EpollService* service = static_cast<EpollService*>(event.data.ptr);
            if (event.events & EPOLLIN) {
                service->onEpollIn();
            }
            if (event.events & EPOLLOUT) { //unlikely
                service->onEpollOut();
            }
            if (event.events & (EPOLLERR|EPOLLHUP|EPOLLRDHUP)) { //unlikely
                service->onEpollError();
            }
        }
    }
    std::cout << *this << " stopped\n";
}

std::ostream& operator<<(std::ostream& os, const EpollActiveObject& obj) {
    os << "EpollActiveObject<" << obj.name_ << '@' << obj.epoll_ << ">(" << &obj << ')';
}
