#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[])
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sin = { 0 };
    sin.sin_port = htons(8080);
    bind(listenfd, (struct sockaddr*)&sin, sizeof(sin));

    listen(listenfd, 10);

    fd_set fds, readfds;
    FD_ZERO(&fds);
    FD_SET(listenfd, &fds);
    int nfd = listenfd;

    struct timeval tv, timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    while (1) {
        readfds = fds;
        tv = timeout;
        int ready = select(nfd+1, &readfds, NULL, NULL, &tv);
        if (ready < 0) { perror("select"); return -1; }
        else if (ready == 0) { printf("%ld hellO, anybody HOME?\n", time(0)); continue; }

        for (int fd=0; fd<nfd+1; ++fd) {
            if (FD_ISSET(fd, &readfds)) {
                if (fd == listenfd) {
                    int connfd = accept(listenfd, NULL, NULL);
                    printf("got connection %d\n", connfd);
                    FD_SET(connfd, &fds);
                    if (connfd > nfd) nfd = connfd;
                } else {
                    char buf[24];
                    int sz = read(fd, buf, sizeof(buf));
                    if (sz > 0) write(fd, buf, sz); //need check write full message
                    else if (sz == 0) {
                        printf("%d:dropped\n", fd);
                        FD_CLR(fd, &fds);
                        if (fd == nfd) --nfd;
                    } else {
                        perror("read");
                        return -1;
                    }
                }
            }
        }
    }
}
