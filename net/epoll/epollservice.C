#include "epollservice.H"
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>

bool TcpConnection::initialize() {
    return true;
}

void TcpConnection::onEpollIn() {
}

void TcpConnection::onEpollError() {
}

void TcpConnection::send(const char* msg, size_t len) {
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

TcpConnection* EpollActiveObject::createTcpConnection(const std::string& addr) {
    return nullptr;
}

TcpConnectionServer* EpollActiveObject::createTcpConnectionServer(unsigned port) {
    return nullptr;
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
