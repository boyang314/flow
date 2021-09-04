#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <thread>
#include <iostream>

#define futex(...) syscall(SYS_futex, __VA_ARGS__);

static int futex_wait(uint32_t* uaddr, uint32_t val) {
    int ret = futex(uaddr, FUTEX_WAIT, val, NULL, NULL, 0);
    if (-1 == ret) perror("futex_wait");
    return ret;
}

static int futex_wake(uint32_t* uaddr, uint32_t n) {
    int ret = futex(uaddr, FUTEX_WAKE, n, NULL, NULL, 0);
    if (-1 == ret) perror("futex_wake");
    return ret;
}

uint32_t evt = 0;

void event_loop() {
    while(evt < 20) {
        int ret = futex_wait(&evt, evt);
        if (-1 == ret) perror("futex_wait");
        std::cout << "loop:\t" << ret << " evt:" << evt << '\n';
        ++evt;
    }
}

void event_trigger() {
    while(evt < 20) {
        int ret = futex_wake(&evt, 1);
        if (-1 == ret) perror("futex_wake");
        std::cout << "trig:\t" << ret << " evt:" << evt << '\n';
        sleep(1);
    }
}

int main() {
    uint32_t fut1 = 123;
    uint32_t fut2 = 456;

    int ret;
    std::thread t1([&] { 
            fut1 = 456;
            fut2 = 456;
            sleep(2); futex_wake(&fut1, 1);
            sleep(2); futex_wake(&fut2, 1);
            fut1 = 123;
            fut2 = 123;
            sleep(2); futex_wake(&fut1, 1);
            sleep(2); futex_wake(&fut2, 1);
            });

    std::thread t2(event_loop);
    std::thread t3(event_trigger);

    sleep(1);
    ret = futex_wait(&fut1, 456);
    std::cout << ret << " free1\n";
    ret = futex_wait(&fut2, 456);
    std::cout << ret << " free2\n";
    ret = futex_wait(&fut1, 123);
    std::cout << ret << " free1\n";
    ret = futex_wait(&fut2, 123);
    std::cout << ret << " free2\n";

    t1.join();
    t2.join();
    t3.join();
}
