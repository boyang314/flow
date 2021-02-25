#include "epollservice.H"
#include "utils.H"

#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <cassert>

//TcpConnection

TcpConnection::TcpConnection(int epoll, const std::string& peerAddr, StreamListener* listener) 
: EpollService(epoll), addr_(peerAddr), listener_(listener) {
    assert(epoll != 0);
    assert(listener != nullptr);
    memset(&local_, 0, sizeof(local_));
    memset(&remote_, 0, sizeof(remote_));
}

TcpConnection::TcpConnection(int epoll, int fd, const struct sockaddr_in& remote, StreamListener* listener) 
: EpollService(epoll), addr_(SysUtil::getReadableTcpAddress(remote)), fd_(fd), listener_(listener) {
    assert(epoll != 0);
    assert(listener != nullptr);
    memset(&local_, 0, sizeof(local_));
    memset(&remote_, 0, sizeof(remote_));
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
        //tcpSendBufSize, tcpReceiveBufSize, tcpKeepAliveTimeSec, connectTimeOutSec
        fd_ = SysUtil::createTcpClientFd(addr_, 0, 0, 0, 1, remote_);
        if (0 > fd_) {
            std::cerr << "failed to create tcp client fd to " << addr_ << std::endl;
            fd_ = 0; return false;
        }
    }
    /*
    socklen_t len = sizeof(local_);
    if (getsockname(fd_, (sockaddr*)&local_, &len) < 0) {
        //error
        ::close(fd_); fd_ = 0; return false;
    }
    */
    if (!SysUtil::registerToEpoll(epoll_, fd_, EPOLLIN|EPOLLERR|EPOLLHUP|EPOLLRDHUP, static_cast<EpollService*>(this))) {
        std::cerr << "TcpConn::init failed to register to epoll" << std::endl; 
        ::close(fd_); fd_ = 0; return false;
    }
    listener_->onConnect(this);
    return true;
}

void TcpConnection::onEpollIn() {
    std::cout << "TcpConn onEpollIn\n";
    const unsigned max_msg_size = 1024;
    char buf[max_msg_size];
    size_t sizeProcessed = 0;
    while(sizeProcessed < 100 * max_msg_size) {
        if (0 == fd_) return; //socket closed by listener or not properly removed from epoll
        if (closing_) {
            onEpollError(); return;
        }
        //throtling required?
        int ret = recv(fd_, buf, sizeof(buf), 0);
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
                break;
        }
    }
}

void TcpConnection::onEpollError() {
    std::cout << "TcpConn onEpollError\n";
    int fd = fd_;
    fd_ = 0;
    if (0 < fd) {
        SysUtil::removeFromEpoll(epoll_, fd);
        ::close(fd); //log error
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
    os << '<' << obj.fd_ << '@' << &obj << '|' << SysUtil::getReadableTcpAddress(obj.local_) << '|' << obj.addr_ << '>';
}

//TcpConnectionServer

TcpConnectionServer::TcpConnectionServer(int epoll, unsigned port, StreamListener* handler) 
: EpollService(epoll), port_(port), handler_(handler) {
    assert(epoll != 0);
    assert(handler != nullptr);
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
    //set fd nonblocking
    //set fd sendBufSize
    //set fd recvBufSize
    //set fd tcpKeepAlive

    return createServerConnection(epoll_, fd, remote, listener);
}

TcpConnection* TcpConnectionServer::createServerConnection(int epoll, int fd, const struct sockaddr_in& remote, StreamListener* listener) {
    TcpConnection* newConn = new TcpConnection(epoll, fd, remote, listener);
    if (!newConn->initialize()) {
        std::cerr << "failed to creat tcp connection" << std::endl;
        delete newConn; return nullptr;
    }
    return newConn;
}

void TcpConnectionServer::close() {
    std::cout << "closed server port " << port_ << std::endl;
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
    //sendBufSize, recvBufSize
    fd_ = SysUtil::createTcpServerFd(port_, 64*1024, 64*1024, local_); //note local_ uninitialized
    if (fd_ < 0) {
        std::cerr << "Failed to create TcpServer@" << port_ << std::endl;
        return false;
    }
    if (!SysUtil::registerToEpoll(epoll_, fd_, EPOLLIN|EPOLLERR|EPOLLHUP, static_cast<EpollService*>(this))) {
        std::cerr << "Failed to register to epoll TcpServer@" << port_ << std::endl;
        ::close(fd_); fd_ = 0; return false;
    }
    std::cout << "TcpSever initialized@" << port_ << std::endl;
    return true;
}

void TcpConnectionServer::onEpollIn() {
    std::cout << "TcpServer onEpollIn\n";
    accept(handler_);
}

void TcpConnectionServer::onEpollError() {
    std::cout << "TcpServer onEpollError\n";
    close();
}

//UdpUnicast

bool UdpUnicast::initialize() {
    return true;
}

void UdpUnicast::onEpollIn() {
}

void UdpUnicast::onEpollError() {
}

void UdpUnicast::send(const char* msg, size_t len) {
}

//McastSender

bool McastSender::initialize() {
    return true;
}

void McastSender::onEpollIn() {
}

void McastSender::onEpollError() {
}

void McastSender::send(const char* msg, size_t len) {
}

bool McastReceiver::initialize() {
    return true;
}

//McastReceiver

void McastReceiver::onEpollIn() {
}

void McastReceiver::onEpollError() {
}

//EpollActiveObject

EpollActiveObject::EpollActiveObject(const std::string& name) : name_(name) {
    epoll_ = epoll_create(max_epoll_events_);
    //error check epoll_ <= 0
    std::cout << *this << " initialized\n";
}

TcpConnection* EpollActiveObject::createTcpConnection(const std::string& peerAddr, StreamListener* listener) {
    TcpConnection* conn = new TcpConnection(epoll_, peerAddr, listener);
    if (!conn->initialize()) { delete conn; return nullptr; }
    std::cout << "createTcpConnection " << peerAddr << std::endl;
    return conn;
}

TcpConnectionServer* EpollActiveObject::createTcpConnectionServer(unsigned port, StreamListener* handler) {
    TcpConnectionServer* server = new TcpConnectionServer(epoll_, port, handler);
    if (!server->initialize()) { delete server; return nullptr; }
    std::cout << "createTcpConnectionServer port:" << port << std::endl;
    return server;
}

UdpUnicast* EpollActiveObject::createUdpUnicast(const std::string& addr) {
    return nullptr;
}

McastSender* EpollActiveObject::createMcastSender(const std::string& addr) {
    return nullptr;
}

McastReceiver* EpollActiveObject::createMcastReceiver(const std::string& addr) {
    return nullptr;
}

void EpollActiveObject::start() {
    if (running_) return;
    thread_ = std::thread{[this](){ this->run();}};
    running_ = 1;
}

void EpollActiveObject::stop() {
    if (running_) {
        thread_.join();
        running_ = 0;
    }
}

void EpollActiveObject::run() {
    std::cout << *this << " started\n";
    //pin this thread to core and set realtime if possible
    int counter = 0;
    while(1) { //unlikely thread interrupted
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
