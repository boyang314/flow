#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>

#define NFDS 3 

uint16_t getIndex(const struct pollfd pfds[], const uint16_t count) {
    for(uint16_t i=0; i<count; ++i) {
        if (pfds[i].fd == 0) return i;
    }
    return count;
}

int main(int argc, char *argv[])
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sin = { 0 };
    sin.sin_port = htons(8080);
    bind(listenfd, (struct sockaddr*)&sin, sizeof(sin));
    listen(listenfd, 10);

    struct pollfd pfds[NFDS] = { 0 };
    pfds[0].fd = listenfd;
    pfds[0].events = POLLIN;
    uint16_t count = 1;

    while (1) {
        int ready = poll(pfds, count, 10000);
        if (ready < 0) { perror("poll"); return -1; }
        else if (ready == 0) { printf("%ld hellO, anybody HOME?\n", time(0)); continue; }

        if (pfds[0].revents & POLLIN) {
            int connfd = accept(listenfd, NULL, NULL);
            uint16_t idx = getIndex(pfds, count);
            if (idx >= NFDS) {
                const char msg[] = "exceed max conns";
                printf("%s drop %d\n", msg, connfd);
                write(connfd, msg, sizeof(msg));
                close(connfd);
                continue;
            }
            if (idx == count) ++count;
            pfds[idx].fd = connfd;
            pfds[idx].events = POLLIN;
            //pfds[idx].events = POLLIN | POLLHUP; //not working in WSL
            pfds[0].revents = 0;
            printf("new connection conn:idx %d:%d fd:revents %d:%d\n", connfd, idx, pfds[0].fd, pfds[0].revents);
        }

        for (auto& pfd : pfds) {
            if (pfd.fd == 0 || pfd.revents == 0) continue;
            printf("fd:revents %d:%d\n", pfd.fd, pfd.revents);
            /*  //not working in WSL
            if (pfd.revents & POLLHUP) {
                printf("fd %d dropped\n", pfd.fd);
                close(pfd.fd); //do a final read before close
                pfd.fd = 0; //zero out full structure
                continue;
            } */
            if (pfd.revents & POLLIN) {
                char buf[24];
                int sz = read(pfd.fd, buf, sizeof(buf));
                if (sz < 0) { perror("read"); return -1; }
                else if (sz == 0) {
                    printf("read0 fd %d dropped\n", pfd.fd);
                    close(pfd.fd);
                    pfd.fd = 0;
                    continue;
                }
                write(pfd.fd, buf, sz); //need check write full message
            }
            else {
                printf("unexpected fd:revents %d:%d\n", pfd.fd, pfd.revents);
            }
        }
    }
}
