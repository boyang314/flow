#include <sys/signalfd.h>
#include <signal.h>
#include <sys/timerfd.h>
#include <time.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>

#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <thread>
#include <deque>
#include <functional>

#include <fcntl.h>

typedef std::function<void()> Task;
typedef std::deque<Task> TaskQueue;
TaskQueue tqueue;

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#ifdef DEBUG
#define debug(fmt, ...) \
    do { fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)
#define trace(fmt, ...) \
    do { fprintf(stderr, "%s:%d:%s():%ld: " fmt, __FILE__, \
            __LINE__, __func__, gettid(), __VA_ARGS__); } while (0)
#else
#define debug(fmt, ...)
#define trace(fmt, ...)
#endif 

#define MAX_EVENTS 10
struct epoll_event ev, events[MAX_EVENTS];

void genEvent(int efd, uint64_t u) {
    ssize_t s = write(efd, &u, sizeof(uint64_t));
    if (s != sizeof(uint64_t)) handle_error("write");
    //trace("write %lu to efd %d\n", u, efd);
}

uint64_t handleEvent(int efd) {
    uint64_t u;
    ssize_t s = read(efd, &u, sizeof(uint64_t));
    if (s != sizeof(uint64_t)) handle_error("read");
    /* O_NONBLOCK
    if (s == -1 && errno != EAGAIN) handle_error("read");
    if (s = sizeof(uint64_t) && errno != EAGAIN)
    */
    trace("handleEvent:%lu\n", u);
    return u;
}

uint64_t handleTimer(int tfd) {
    uint64_t u;
    ssize_t s = read(tfd, &u, sizeof(uint64_t));
    if (s != sizeof(uint64_t)) handle_error("read");
    itimerspec cvalue;
    int ret = timerfd_gettime(tfd, &cvalue);
    if (ret == -1) handle_error("timer_gettime");
    trace("handleTimer:%lu sec:%ld nsec:%ld\n", u, cvalue.it_value.tv_sec, cvalue.it_value.tv_nsec);
    return u;
}

int dfd; //done for day
void handleSignal(int sfd) {
    struct signalfd_siginfo fdsi;
    ssize_t s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
    if (s != sizeof(struct signalfd_siginfo)) handle_error("read");
    if (fdsi.ssi_signo == SIGINT) {
        debug("Got SIGINT\n");
        genEvent(dfd, 1);
    } else if (fdsi.ssi_signo == SIGQUIT) {
        debug("Got SIGQUIT\n");
    } else {
        debug("Read unexpected signal\n");
    }
}

bool ready = true;

void doWork() {
    while (ready) {
        if (tqueue.size() > 0) {
            tqueue.front()();
            tqueue.pop_front();
        } else {
            sleep(1);
        }
    }
    debug("drain\n");
    /*
    while(tqueue.size() > 0) {
        tqueue.front()();
        tqueue.pop_front();
    }
    */
}

int main(int argc, char *argv[])
{
    //int efd = eventfd(0, 0);
    int efd = eventfd(0, O_NONBLOCK);
    if (efd == -1) handle_error("eventfd");

    dfd = eventfd(0, 0);
    if (dfd == -1) handle_error("eventfd");

    int tfd = timerfd_create(CLOCK_REALTIME, 0);
    if (tfd == -1) handle_error("timerfd_create");

    struct timespec now;
    struct itimerspec new_value;
    if (clock_gettime(CLOCK_REALTIME, &now) == -1) handle_error("clock_gettime");
    /*
    new_value.it_value.tv_sec = now.tv_sec + 1;
    new_value.it_value.tv_nsec = now.tv_nsec;
    new_value.it_interval.tv_sec = 1;
    new_value.it_interval.tv_nsec = 100000;
    */
    new_value = { { 1, 100000 }, { now.tv_sec+1, now.tv_nsec } }; //does not work
    //new_value.it_interval = { 1, 100000 };
    //new_value.it_value = { now.tv_sec+1, now.tv_nsec };
    if (timerfd_settime(tfd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1) handle_error("timerfd_settime");

    sigset_t mask;
    //sigfillset(&mask);
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);

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
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, &ev) == -1) handle_error("epoll_ctl: add sfd");

    uint64_t magic = UINT64_MAX - 1;
    //std::thread t1([&] { for(int i=0; i<100; ++i) { genEvent(efd, i); sleep(1); } genEvent(efd, magic); });

    const int NWORKERS = 2;
    std::thread workers[NWORKERS];
    printf("sizeof workers %lu\n", sizeof(workers));
    for (int i=0; i<NWORKERS; ++i) {
        workers[i] = std::thread([&] { for(int i=0; i<10; ++i) { genEvent(efd, i); sleep(1); }});
    }

    std::thread dwork(doWork);

    debug("ready:%d\n", ready);
    trace("ready:%d\n", ready);

    while (ready) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) handle_error("epoll_wait");

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == efd) {
                //tqueue.push_back([&] { handleEvent(efd); });
                uint64_t e = handleEvent(efd);
                if (e == magic) { ready = false; break; }
            } else if (events[n].data.fd == tfd) {
                tqueue.push_back([&] { handleTimer(tfd); });
                //handleTimer(tfd);
            } else if (events[n].data.fd == sfd) {
                handleSignal(sfd);
                //tqueue.push_back([&] { handleSignal(sfd); }); //not right handleSignal in thread
            } else if (events[n].data.fd == dfd) {
                ready = false;
                tqueue.push_back([&] { printf("hello world!\n"); }); //did not trigger, add final drain
                sleep(3);
                break;
            }
        }
    }
    printf("quit mainloop\n");

    //tqueue.push_back([] { exit(0); }); //handleEvent already blocking on read !!!

    for (int i=0; i<NWORKERS; ++i) {
        workers[i].join();
    }
    dwork.join();
}

