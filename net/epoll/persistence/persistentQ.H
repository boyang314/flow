#pragma once

#include <cstdint>
#include <string>
#include <mutex>
#include <thread>

struct MessageHeader {
    uint64_t type_;
    size_t size_;
    uint64_t key_;
    uint64_t time_;
    uint64_t srcTime_;
    uint64_t sendTime_;
    uint64_t sessionId_;
    uint64_t sequence_;
    uint64_t appsequence_;
    uint64_t bytesequence_;
    uint64_t appbytesequence_;
    size_t const sizeOfPayload() const { return sizeof(MessageHeader); }
    static uint64_t id() { return 1234567; }
    char buffer_[24];
};

class persistentQ {
    struct SessionInfo {
        std::array<uint64_t, 3> info_;
        uint64_t const& writeIndex() const { return info_[0]; }
        uint64_t& writeIndex() { return info_[0]; }
        uint64_t& readIndex() { return info_[1]; }
        uint64_t& seq() { return info_[2]; }
    };
    std::string fileName_;
    size_t fileSize_{1024*1024*1024*1024l}, windowSize_{1024*1024*128l}, pageSize_{}, memoryIndex_{};
    int16_t memoryNode_{-1}; //numa
    bool preloadPages_{true};
    int indexFileFd_{-1}, streamFileFd_{-1};
    char *indexBuffer_{nullptr}, *streamBuffer_{nullptr};
    uint64_t sessionId_{};
    std::mutex lock_;
    std::thread memoryManager_;
    bool done_{false};
public:
    template<typename Msg>
    struct autoCommit {
        persistentQ& q_;
        MessageHeader* header_;
        size_t sz_;
        autoCommit(persistentQ& q, Msg const& msg) : q_(q), header_(reinterpret_cast<MessageHeader*>(q_.beginWrite())), sz_(sizeof(MessageHeader) + msg.sizeOfPayload()) {
            header_->type_ = Msg::id();
            header_->size_ = sz_;
            //msg.toMessage(*header);
            header_->key_ = 123;
        }
        ~autoCommit() {
            header_->time_ = header_->srcTime_ = header_->sendTime_ = 0;
            header_->sessionId_ = q_.sessionId();
            header_->sequence_ = header_->appsequence_ = q_.sessionInfo().seq();
            header_->bytesequence_ = header_->appbytesequence_ = q_.writeIndex();
            q_.commitWrite(header_->size_);
        }
        /*
        MessageHeader* operator->() { return header_; }
        //MessageHeader& operator*() & { return *header_; }
        MessageHeader& operator*() { return *(this.operator->()); }
        */
    };
        
    persistentQ();

    bool open();
    bool isOpen();
    void close();

    SessionInfo& sessionInfo() { return *(SessionInfo*)indexBuffer_; }
    SessionInfo const& sessionInfo() const { return *(const SessionInfo*)indexBuffer_; }
    size_t writeIndex() const { return sessionInfo().writeIndex(); }
    size_t writeIndex() { return sessionInfo().writeIndex(); }
    uint64_t sessionId() { return sessionId_; }

    template<typename Msg>
    autoCommit<Msg> write(Msg const& msg) {
        return {*this, msg};
    }

private:
    bool openIndexFile();
    bool openStreamFile();
    bool mapMemory();
    bool refreshMemory();
    char *beginWrite();
    void commitWrite(size_t sz);
    void removeOldFiles();
};

