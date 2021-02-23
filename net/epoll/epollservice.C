#include "epollservice.H"
#include "utils.H"

#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <cassert>

TcpConnection::TcpConnection(int epoll, const std::string& peerAddr, StreamListener* listener) 
: EpollService(epoll), addr_(peerAddr), listener_(listener) {
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
    return true;
}

void TcpConnection::onEpollIn() {
}

void TcpConnection::onEpollError() {
}

ssize_t TcpConnection::send(const char* msg, size_t len) {
    return 0;
}

std::ostream& operator<<(std::ostream& os, const TcpConnection& obj) {
    os << '<' << obj.fd_ << '@' << &obj << '|' << SysUtil::getReadableTcpAddress(obj.local_) << '|' << obj.addr_ << '>';
}

TcpConnectionServer::TcpConnectionServer(int epoll, unsigned port, StreamListener* listener) 
: EpollService(epoll) {
    //check parameters
}

bool TcpConnectionServer::initialize() {
    return true;
}

void TcpConnectionServer::onEpollIn() {
}

void TcpConnectionServer::onEpollError() {
}

bool UdpUnicast::initialize() {
    return true;
}

void UdpUnicast::onEpollIn() {
}

void UdpUnicast::onEpollError() {
}

void UdpUnicast::send(const char* msg, size_t len) {
}

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

void McastReceiver::onEpollIn() {
}

void McastReceiver::onEpollError() {
}

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
            std::cout << " timeout...\n";
            if (counter >= 10) break;
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
