struct Header {
    uint32_t len;
    uint32_t seqNo;
};

inline bool hasCompleteMsg(char* offset, size_t remaining) {
    return remaining > sizeof(Header) && remaining >= (Header*)offset->len + sizeof(Header);
}

inline void processMsg(char*& offset, size_t& remaining) {
    printf("%s", offset+4);
    size_t unit = (Header*)offset->len + sizeof(Header);
    offset += unit;
    remaining -= unit;
}

inline void processBuffer(char*& curOffset, size_t& remaining) {
    while(hasCompleteMsg(curOffset, remaining)) {
        processMsg(curOffset, remaining);
    }
}

#define LEN 4096
char buf[LEN];
size_t remaining=0;

while(1) {
    poll();
    int n = read(fd, buf+remaining, LEN-remaining);
    remaining += n;
    char* curOffset = buf;
    processBuffer(curOffset, remaining);
    if (remaining == 0) break;
    memove(buf, curOffset, remaining);
}
