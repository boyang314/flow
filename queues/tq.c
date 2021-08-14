#include <atomic>
#include <thread>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sched.h>

//#define SIZE 1024 //working without usleep
#define SIZE 8192
#define SMASK SIZE-1

#define etype int*

class mpsc {
    etype buf[SIZE]; //should use atomic type
    int head{0};
    std::atomic_int tail{0};
public:
    std::atomic_int ecount_{0};
    std::atomic_int dcount_{0};
public:
    mpsc() { memset(buf, 0, sizeof(etype)*SIZE); }

    bool enq(etype e) {
        int ctail = tail.fetch_add(1);
        int idx = ctail&SMASK;
        if (buf[idx] == NULL) {
            buf[idx] = e;
            ++ecount_;
            return true;
        } else {
            tail.fetch_add(-1);
            return false;
        }
    }

    bool deq(etype& e) {
        int idx = head&SMASK;
        e = buf[idx];
        if (e != NULL) {
            buf[idx] = NULL; 
            ++head;
            ++dcount_;
            return true;
        }
        return false;
    }
};

int global = 1;
mpsc q;

void enqueue() {
    for(int i=0; i<SIZE; ++i) {
        //while(!q.enq(&global)); //yield to be nice
        while(!q.enq(&global)) { usleep(1000); }
        //while(!q.enq(&global)) { sched_yield(); } //still broken with yield
    }
    std::cout << "ecount:" << q.ecount_ << '\n';
}

void dequeue() {
    int sum=0;
    for(int i=0; i<4*SIZE; ++i) {
        int* tmp=0;
        if(q.deq(tmp)) {
            sum += *tmp;
            if (i > 4*SIZE-5) std::cout << "\nsum:" << sum << " dcount:" << q.dcount_ << '\n';
        } else {
            --i;
            //sched_yield();
        }
    }
}

int main() {
    std::thread t1(enqueue);
    std::thread t3(enqueue);
    std::thread t4(enqueue);
    std::thread t5(enqueue);
    std::thread t2(dequeue);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
}
