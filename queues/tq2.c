#include <atomic>
#include <thread>
#include <iostream>

#define SIZE 4096
#define SMASK SIZE-1

class mpsc {
    std::atomic<int*> buf[SIZE]; //should use atomic type
    int head{0};
    std::atomic_int tail{0};
public:
    bool enq(int* e) {
        int ntail = tail.fetch_add(1);
        if (buf[ntail & SMASK].load() == NULL) {
            buf[ntail & SMASK].store(e);
            return true;
        } else {
            tail.fetch_add(-1);
            return false;
        }
    }

    bool deq(int*& e) {
        e = buf[head & SMASK].load();
        if (e != NULL) {
            buf[head & SMASK].store(NULL); 
            ++head;
            //__asm__ __volatile__("":::"memory");
            return true;
        }
        return false;
    }
};

int global = 1;
mpsc q;

void enqueue() {
    /*
    int sum=0;
    while(sum<SIZE) {
        if (q.enq(&global)) ++sum;
    }
    */
    int sum=0;
    for(int i=0; i<SIZE; ++i) {
        while(!q.enq(&global));
        ++sum;
    }
    std::cout << "ensum:" << sum << '\n';
}

void dequeue() {
    int* tmp;
    int sum=0;
    for(int i=0; i<2*SIZE; ++i) {
        //if(q.deq(tmp)) sum+=*tmp;
        while(!q.deq(tmp));
        sum += *tmp;
        std::cout << "\nsum:" << sum << '\n';
    }
}

int main() {
    std::thread t1(enqueue);
    std::thread t3(enqueue);
    //std::thread t4(enqueue);
    //std::thread t5(enqueue);
    std::thread t2(dequeue);
    t1.join();
    t2.join();
    t3.join();
    //t4.join();
    //t5.join();
}
