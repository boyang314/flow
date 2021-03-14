#include "persistedBuffer.H"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

#include <cstring>
#include <iostream>

persistedBuffer::persistedBuffer(uint32_t defaultBufferSize) : messageBuffer_(NULL), messageBufferSize_(0), msgCount_(0), writePos_(0), fd_(-1) {
    fd_ = shm_open("/persistedBuffer", O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
    if (fd_ == -1) {
        perror("error in shm_open");
        exit(-1);
    }

    if (ftruncate(fd_, defaultBufferSize) == -1) {
        perror("error in ftruncate");
        exit(-1);
    }

    void *shm_addr = mmap(0, defaultBufferSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd_, 0);
    if (shm_addr == MAP_FAILED) {
        perror("error mapping file");
        exit(-1);
    }

    messageBuffer_ = (char*)shm_addr;
    messageBufferSize_ = defaultBufferSize;
}

persistedBuffer::~persistedBuffer() {
    munmap((void*)messageBuffer_, messageBufferSize_);
    messageBuffer_ = NULL;
    if (fd_ != -1) close(fd_);
    //shm_unlink, no we still need to reload this file
}

uint32_t persistedBuffer::add(char *buf, uint16_t size) {
    if (messageBufferSize_ - writePos_ < size) resize();
    memcpy(messageBuffer_ + writePos_, buf, size);
    writePos_ += size;
    ++msgCount_;
    return writePos_;
}

void persistedBuffer::clean() {
    memset(messageBuffer_, 0, messageBufferSize_);
    writePos_ = 0;
    msgCount_ = 0;
}

void persistedBuffer::resize() {
    //check less than max allowed buffer size, action to take??
    uint32_t newBufferSize = messageBufferSize_ * 2;
    if (ftruncate(fd_, newBufferSize) == -1) {
        perror("error in ftruncate");
        exit(-1);
    }
    void *newBuffer = mremap((void*)messageBuffer_, messageBufferSize_, newBufferSize, MREMAP_MAYMOVE);
    if (newBuffer == (void*)-1) {
        perror("error on mremap");
        exit(-1);
    }
    messageBuffer_ = (char*)newBuffer;
    messageBufferSize_ = newBufferSize;
}

