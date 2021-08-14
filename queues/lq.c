#include <thread>
#include <mutex>
#include <iostream>
#include <unistd.h>
#include <stdint.h>

#define SIZE 4096
#define SMASK SIZE-1

class lq {
    int* buf[SIZE]; //should use atomic type
    int head{0};
    int tail{0};
    //uint32_t ecount_{0};
    //uint32_t dcount_{0};
    std::mutex mtx;
public:
    bool enq(int* e) {
        std::lock_guard<std::mutex> guard(mtx);
        int ntail = (tail+1) & SMASK;
        if (head == ntail) return false; 
        buf[tail] = e; //note update buf[tail] then update tail, avoid leave null in it
        tail = ntail;
        //++ecount_;
        return true;
    }

    bool deq(int*& e) {
        std::lock_guard<std::mutex> guard(mtx);
        if (head == tail) return false;
        e = buf[head];
        head = (head+1) & SMASK;
        //++dcount_;
        return true;
    }
    /*
    uint32_t ecount() { 
        std::lock_guard<std::mutex> guard(mtx);
        return ecount_;
    }
    uint32_t dcount() { 
        std::lock_guard<std::mutex> guard(mtx);
        return dcount_; 
    }
    */
};

int global = 1;
lq q;

void enqueue() {
    int sum=0;
    for(int i=0; i<SIZE; ++i) {
        if(q.enq(&global)) {
            ++sum;
        } else {
            --i;
        }
    }
    //std::cout << "ensum:" << sum << " ecount:" << q.ecount() << '\n';
}

void dequeue(int i) {
    int sum=0;
    while (1) {
        int* tmp=0;
        if (q.deq(tmp)) {
            sum += *tmp;
            //std::cout << "sum:" << sum << " dcount:" << q.dcount() << '\n';
        } else {
            //std::cout << "dequeue empty\n";
            //usleep(100000);
            sleep(1);
            std::cout << i << " so far dsum:" << sum << '\n';
        }
    }
}

int main() {
    std::thread t1(enqueue);
    std::thread t2(dequeue, 2);
    std::thread t3(enqueue);
    std::thread t4(enqueue);
    std::thread t5(enqueue);
    std::thread t6(dequeue, 6);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
}
