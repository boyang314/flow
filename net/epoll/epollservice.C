#include "epollservice.H"

void TcpConnection::onEpollIn() {
}

void TcpConnection::onEpollError() {
}

void TcpConnection::send(const char* msg, size_t len) {
}

void TcpConnectionServer::onEpollIn() {
}

void TcpConnectionServer::onEpollError() {
}

void UdpUnicast::onEpollIn() {
}

void UdpUnicast::onEpollError() {
}

void UdpUnicast::send(const char* msg, size_t len) {
}

void McastSender::onEpollIn() {
}

void McastSender::onEpollError() {
}

void McastSender::send(const char* msg, size_t len) {
}

void McastReceiver::onEpollIn() {
}

void McastReceiver::onEpollError() {
}

EpollActiveObject::EpollActiveObject(const std::string& name) : name_(name) {
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

void EpollActiveObject::start() {}
void EpollActiveObject::stop() {}
void EpollActiveObject::run() {}
