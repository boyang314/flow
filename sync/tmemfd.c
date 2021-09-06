//#include <sys/memfd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <thread>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
} while (0)

#define memfd_create(name) syscall(__NR_memfd_create, name, 0)
#define gettid() syscall(SYS_gettid)

void peek(int& fd) {
    off_t oft = 0;
    const size_t sz = 13;
    char buf[sz];
    for (int i=0; i<1000000; ++i) {
        //bzero(buf, sz);
        //lseek(fd, oft, SEEK_SET);
        //int ret = read(fd, buf, sizeof(buf));
        int ret = pread(fd, buf, sizeof(buf), oft);
        if (ret == -1) errExit("read");
        if ((i+1)%100000==0) printf("%ld:%d:%s\n", gettid(), i, buf);
        //sleep(1);
        oft += sz;
    }
}

int main(int argc, char *argv[])
{
    const char *name = "";
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

    std::thread t1([&fd] { peek(fd); });
    std::thread t2([&fd] { peek(fd); });

    /* Keep running, so that the file created by memfd_create()
       continues to exist */
    //pause();

    t1.join();
    t2.join();
    exit(EXIT_SUCCESS);
}

