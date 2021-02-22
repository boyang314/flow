#pragma once

#include <string>
#include <thread>

class EpollService {
public:
    virtual void onEpollOut() {}
    virtual void onEpollIn() = 0;
    virtual void onEpollError() = 0;
};

class TcpConnection : public EpollService {
public:
    virtual void onEpollIn() override;
    virtual void onEpollError() override;
    void send(const char* msg, size_t len);
};

class TcpConnectionServer : public EpollService {
public:
    virtual void onEpollIn() override;
    virtual void onEpollError() override;
};

class UdpUnicast : public EpollService {
public:
    virtual void onEpollIn() override;
    virtual void onEpollError() override;
    void send(const char* msg, size_t len);
};

class McastSender : public EpollService {
public:
    virtual void onEpollIn() override;
    virtual void onEpollError() override;
    void send(const char* msg, size_t len);
};

class McastReceiver : public EpollService {
public:
    virtual void onEpollIn() override;
    virtual void onEpollError() override;
};

class EpollActiveObject {
public:
    EpollActiveObject(const std::string& name);

    TcpConnection* createTcpConnection(const std::string& addr);
    TcpConnectionServer* createTcpConnectionServer(unsigned port);
    UdpUnicast* createUdpUnicast(const std::string& addr);
    McastSender* createMcastSender(const std::string& addr);
    McastReceiver* createMcastReceiver(const std::string& addr);

    void start();
    void stop();

private:
    void run();

    std::string name_;
    std::thread thread_;
};

