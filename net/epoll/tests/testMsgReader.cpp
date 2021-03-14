#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

#include <cstring>
#include <iostream>

#include "messaging/MessageHeader.H"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "readTestMsg persistenceFile\n";
        exit(1);
    }

    int fd = shm_open(argv[1], O_RDONLY, 0);
    if (fd == -1) {
        perror("error in shm_open");
        exit(1);
    }

    off_t fsize = lseek(fd, 0, SEEK_END);
    std::cout << "filesz: " << fsize << '\n';
    lseek(fd, 0, SEEK_SET);
    size_t defaultBufferSize = fsize;

    void *shm_addr = mmap(0, defaultBufferSize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (shm_addr == MAP_FAILED) {
        perror("error mapping file");
        exit(1);
    }

    char* messageBuffer = (char*)shm_addr;
    uint32_t readPos = 0;
    while (readPos < defaultBufferSize) {
        MessageHeader* header = (MessageHeader*)(messageBuffer + readPos);
        auto sz = header->getMessageSize();
        if (sz == 0) break;
        std::cout << "readPos:" << readPos << ':' << *header << std::endl;
        readPos += sz;
    }

    munmap((void*)messageBuffer, defaultBufferSize);
    if (fd != -1) close(fd);
}

