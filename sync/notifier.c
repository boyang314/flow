#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <thread>
#include <iostream>

static int futex(uint32_t *uaddr, int futex_op, uint32_t val, const struct timespec *timeout, uint32_t *uaddr2, uint32_t val3)
{
    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

static int futex_wait(uint32_t* uaddr, uint32_t val) {
    return futex(uaddr, FUTEX_WAIT, val, NULL, NULL, 0);
}

static int futex_wake(uint32_t* uaddr, uint32_t n) {
    return futex(uaddr, FUTEX_WAKE, n, NULL, NULL, 0);
}

class fbarrier {
public:
    fbarrier() : val_(0) {}
    void signal() {
        ++val_;
        futex_wake(&val_, UINT32_MAX);
    }
    void wait() {
        futex_wait(&val_, val_);
    }
private:
    uint32_t val_;
};

int main() {
    fbarrier ev;
    std::thread t1([&ev] { sleep(1); ev.signal(); sleep(2); ev.signal(); });
    ev.wait();
    std::cout << "free1\n";
    ev.wait();
    std::cout << "free2\n";
    t1.join();
}
