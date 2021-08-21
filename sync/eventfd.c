#include <sys/signalfd.h>
#include <signal.h>
#include <sys/timerfd.h>
#include <time.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <thread>

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define MAX_EVENTS 10
struct epoll_event ev, events[MAX_EVENTS];

void genEvent(int efd, uint64_t u) {
    ssize_t s = write(efd, &u, sizeof(uint64_t));
    if (s != sizeof(uint64_t)) handle_error("write");
}

uint64_t handleEvent(int efd) {
    uint64_t u;
    ssize_t s = read(efd, &u, sizeof(uint64_t));
    if (s != sizeof(uint64_t)) handle_error("read");
    printf("handleEvent:%lu\n", u);
    return u;
}

uint64_t handleTimer(int tfd) {
    uint64_t u;
    ssize_t s = read(tfd, &u, sizeof(uint64_t));
    if (s != sizeof(uint64_t)) handle_error("read");
    //printf("handleTimer:%lu\n", u);
    return u;
}

int dfd; //done for day
void handleSignal(int sfd) {
    struct signalfd_siginfo fdsi;
    ssize_t s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
    if (s != sizeof(struct signalfd_siginfo)) handle_error("read");
    if (fdsi.ssi_signo == SIGINT) {
        printf("Got SIGINT\n");
        genEvent(dfd, 1);
    } else if (fdsi.ssi_signo == SIGQUIT) {
        printf("Got SIGQUIT\n");
    } else {
        printf("Read unexpected signal\n");
    }
}

int main(int argc, char *argv[])
{
    int efd = eventfd(0, 0);
    if (efd == -1) handle_error("eventfd");

    dfd = eventfd(0, 0);
    if (dfd == -1) handle_error("eventfd");

    int tfd = timerfd_create(CLOCK_REALTIME, 0);
    if (tfd == -1) handle_error("timerfd_create");

    struct timespec now;
    struct itimerspec new_value;
    if (clock_gettime(CLOCK_REALTIME, &now) == -1) handle_error("clock_gettime");
    new_value.it_value.tv_sec = now.tv_sec + 1;
    new_value.it_value.tv_nsec = now.tv_nsec;
    new_value.it_interval.tv_sec = 1;
    new_value.it_interval.tv_nsec = 1000000;
    if (timerfd_settime(tfd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1) handle_error("timerfd_settime");

    sigset_t mask;
    sigfillset(&mask);
    /*
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    */
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) handle_error("sigprocmask");

    int sfd = signalfd(-1, &mask, 0);
    if (sfd == -1) handle_error("signalfd");

    int epollfd = epoll_create1(0);
    if (epollfd == -1) handle_error("epollfd");

    ev.events = EPOLLIN;
    ev.data.fd = efd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, efd, &ev) == -1) handle_error("epoll_ctl: add efd");

    ev.events = EPOLLIN;
    ev.data.fd = dfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, dfd, &ev) == -1) handle_error("epoll_ctl: add dfd");

    ev.events = EPOLLIN;
    ev.data.fd = tfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, tfd, &ev) == -1) handle_error("epoll_ctl: add tfd");

    ev.events = EPOLLIN;
    ev.data.fd = sfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, &ev) == -1) handle_error("epoll_ctl: add tfd");

    //uint64_t magic = UINT64_MAX - 1;
    //std::thread t1([&] { for(int i=0; i<100; ++i) { genEvent(efd, i); sleep(1); } genEvent(efd, magic); });
    std::thread t1([&] { for(int i=0; i<10; ++i) { genEvent(efd, i); sleep(1); }});
    std::thread t2([&] { for(int i=0; i<10; ++i) { genEvent(efd, i); sleep(1); }});

    bool ready = true;
    while (ready) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) handle_error("epoll_wait");

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == efd) {
                handleEvent(efd);
                //uint64_t e = handleEvent(efd);
                //if (e == magic) { ready = false; break; }
            } else if (events[n].data.fd == tfd) {
                handleTimer(tfd);
            } else if (events[n].data.fd == sfd) {
                handleSignal(sfd);
            } else if (events[n].data.fd == dfd) {
                ready = false; break;
            }
        }
    }

    t1.join();
    t2.join();
}

