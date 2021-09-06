//#include <sys/memfd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
} while (0)

#define memfd_create(name) syscall(__NR_memfd_create, name, 0);

int main(int argc, char *argv[])
{
    const char *name = "";
    //char *name = "/tmp/t.t";
    ssize_t len = 1024UL*1024*1024*5;

    //fd = memfd_create(name, MFD_ALLOW_SEALING);
    int fd = memfd_create(name);
    if (fd == -1) errExit("memfd_create");
    if (ftruncate(fd, len) == -1) errExit("truncate");

    printf("PID: %ld; fd: %d; /proc/%ld/fd/%d\n",
            (long) getpid(), fd, (long) getpid(), fd);

    char *addr = (char*)mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) errExit("mmap");

    
    int ret = madvise (addr, len, MADV_SEQUENTIAL);
    if (ret < 0) errExit("madvise");

    //with predefined size and ftruncate we can directly assign
    for (ssize_t i=0; i<len; ++i) 
        addr[i] = i & 0x000000FF;
        
    //with size 0, and no ftruncate, use write to populate
    /*
    const char hello[] = "hello world!";
    for (ssize_t i=0; i<1000000; ++i) 
        write(fd, hello, sizeof(hello));
        */

    ret = madvise (addr, len*3/4, MADV_DONTNEED);
    if (ret < 0) errExit("madvise donotneed");

    /* Keep running, so that the file created by memfd_create()
       continues to exist */
    pause();

    exit(EXIT_SUCCESS);
}

