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
    //if (ftruncate(fd, len) == -1) errExit("truncate");

    printf("PID: %ld; fd: %d; /proc/%ld/fd/%d\n",
            (long) getpid(), fd, (long) getpid(), fd);

    const char hello[] = "hello world!";
    for (ssize_t i=0; i<1000000; ++i) 
        write(fd, hello, sizeof(hello));

    /* Keep running, so that the file created by memfd_create()
       continues to exist */
    pause();

    exit(EXIT_SUCCESS);
}

