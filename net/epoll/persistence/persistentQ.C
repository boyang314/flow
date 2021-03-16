#include "persistentQ.H"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <algorithm>

//#include <numaif.h>

persistentQ::persistentQ() {
    sessionId_ = 123; //current seconds
    std::string fn = "/tmp/persistence";
    fileName_ = fn + "." + std::to_string(sessionId_);
    pageSize_ = getpagesize();
    std::cout << fileName_ << ':' << pageSize_ << '\n';
}

bool persistentQ::open() {
    assert(indexFileFd_ == -1);
    assert(!isOpen());
    if (openIndexFile() && openStreamFile()) {
        removeOldFiles();
        memoryManager_ = std::thread([this](){
                using namespace std::chrono;
                while(!done_) {
                    std::this_thread::sleep_for(1s);
                    refreshMemory();
                }
        });
    }
    return memoryManager_.joinable();
}

bool persistentQ::isOpen() {
    return memoryManager_.joinable() && !done_;
}

void persistentQ::close() {
    done_ = true;
    if (memoryManager_.joinable()) memoryManager_.join();
    if (streamBuffer_ && ::munmap(streamBuffer_, fileSize_)) {
        std::cerr << "munmap streamBuffer failed " << strerror(errno) << '\n';
    }
    streamBuffer_ = nullptr;
    if (indexBuffer_ && ::munmap(indexBuffer_, pageSize_)) {
        std::cerr << "munmap indexBuffer failed " << strerror(errno) << '\n';
    }
    indexBuffer_ = nullptr;
    if (streamFileFd_ > 0) {
        ::close(streamFileFd_);
        streamFileFd_ = -1;
    }
    if (indexFileFd_ > 0) {
        ::close(indexFileFd_);
        indexFileFd_ = -1;
    }
    std::cout << "closing persistentQ\n";
}

bool persistentQ::openIndexFile() {
    std::string fn = "seqNo" + fileName_;
    std::replace(fn.begin(), fn.end(), '/', '.');
    indexFileFd_ = ::shm_open(fn.c_str(), O_CREAT|O_RDWR, 0777);
    if (indexFileFd_ == -1) {
        std::cerr << "shm_open failed " << fn << ':' << strerror(errno) << '\n';
        return false;
    }
    if (::ftruncate(indexFileFd_, pageSize_) == -1) {
        std::cerr << "ftruncate failed " << fn << ':' << strerror(errno) << '\n';
        return false;
    }
    indexBuffer_ = (char*)::mmap(0, pageSize_, PROT_READ|PROT_WRITE, MAP_SHARED, indexFileFd_, 0);
    if (indexBuffer_ == MAP_FAILED) {
        std::cerr << "mmap failed " << fn << ':' << strerror(errno) << '\n';
        return false;
    }
    if (!sessionInfo().seq()) sessionInfo().seq() = 1ul;
    return true;
}

bool persistentQ::openStreamFile() {
    streamFileFd_ = ::open(fileName_.c_str(), O_CREAT|O_RDWR, 0644);
    if (streamFileFd_ == -1) {
        std::cerr << "open failed " << fileName_ << ':' << strerror(errno) << '\n';
        return false;
    }
    if (::ftruncate(streamFileFd_, fileSize_) == -1) {
        std::cerr << "ftruncate failed " << fileName_ << ':' << strerror(errno) << '\n';
        return false;
    }
    return mapMemory();
}

bool persistentQ::mapMemory() {
    assert(streamFileFd_ > 0);
    streamBuffer_ = (char*)::mmap(0, fileSize_, PROT_READ|PROT_WRITE, MAP_SHARED, streamFileFd_, 0);
    if (streamBuffer_ == MAP_FAILED) {
        std::cerr << "mmap failed " << fileName_ << ':' << strerror(errno) << '\n';
        return false;
    }
    /*
    if (memoryNode_ != -1) {
        unsigned long nodeMask = 1ul << memoryNode_;
        if (mbind(streamBuffer_, fileSize_, MPOL_PREFERRED, &nodeMask, sizeof(nodeMask)*CHAR_BIT, MPOL_MF_MOVE) == -1) { // -lnuma
            std::cerr << "mbind failed " << memoryNode_ << ':' << strerror(errno) << '\n';
            return false;
        }
    }
    */
    if (madvise(streamBuffer_, fileSize_, MADV_SEQUENTIAL)==-1) {
        std::cerr << "madvise failed " << fileName_ << ':' << strerror(errno) << '\n';
        return false;
    }
    if (madvise(streamBuffer_, fileSize_, MADV_DONTDUMP)==-1) {
        std::cerr << "madvise failed " << fileName_ << ':' << strerror(errno) << '\n';
        return false;
    }
    memoryIndex_ = writeIndex() & ~(pageSize_ - 1);
    if (preloadPages_) {
        if (madvise(streamBuffer_+memoryIndex_, windowSize_, MADV_WILLNEED)==-1) {
            std::cerr << "madvise failed " << fileName_ << ':' << strerror(errno) << '\n';
            return false;
        }
    }
    return true;
}

bool persistentQ::refreshMemory() {
    if (preloadPages_) {
        int64_t preloadIndex = (writeIndex() / pageSize_ ) * pageSize_;
        if (madvise(streamBuffer_+preloadIndex, windowSize_, MADV_WILLNEED)==-1) {
            std::cerr << "madvise failed " << fileName_ << ':' << strerror(errno) << '\n';
            return false;
        }
        std::cout << "preload " << windowSize_/(1024*1024l) << "MB, preloadIndex " << preloadIndex << " memoryIndex " << memoryIndex_ << " writeIndex " << writeIndex() << '\n';
    }
    size_t bytesToFree = ((writeIndex() - memoryIndex_) / pageSize_) * pageSize_;
    if (bytesToFree > pageSize_ * 2) {
        if (madvise(streamBuffer_+memoryIndex_, bytesToFree, MADV_DONTNEED)==-1) {
            std::cerr << "madvise failed " << fileName_ << ':' << strerror(errno) << '\n';
            return true; //note true
        }
        memoryIndex_ += bytesToFree;
        std::cout << "page out " << bytesToFree << " memoryIndex " << memoryIndex_ << " writeIndex " << writeIndex() << '\n';
    }
    return true;
}

char *persistentQ::beginWrite() {
    //lock.lock
    return streamBuffer_ + writeIndex();
}

void persistentQ::commitWrite(size_t sz) {
    sessionInfo().writeIndex() += sz;
    ++sessionInfo().seq();
    //lock.unlock
}

void persistentQ::removeOldFiles() {
    assert(!fileName_.empty());
    std::cout << "remove " << fileName_ << '\n';
}


