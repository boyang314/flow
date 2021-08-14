#include <atomic>
#include <thread>
#include <iostream>

#define SIZE 4096
#define SMASK SIZE-1

class mpsc {
    int* buf[SIZE]; //should use atomic type
    int head{0};
    std::atomic_int tail{0};
public:
    bool enq(int* e) {
        int ntail = tail.fetch_add(1);
        if (buf[ntail & SMASK] == NULL) {
            buf[ntail & SMASK] = e;
            return true;
        } else {
            tail.fetch_add(-1);
            return false;
        }
    }

    bool deq(int*& e) {
        e = buf[head & SMASK];
        if (e != NULL) {
            buf[head & SMASK] = NULL; 
            ++head;
            return true;
        }
        return false;
    }
};

int global = 1;
mpsc q;

void enqueue() {
    for(int i=0; i<SIZE; ++i) {
        q.enq(&global);
    }
}

void dequeue() {
    int* tmp;
    int sum=0;
    for(int i=0; i<2*SIZE; ++i) {
        //if(q.deq(tmp)) sum+=*tmp;
        while(!q.deq(tmp));
        sum += *tmp;
    }
    std::cout << "\nsum:" << sum << '\n';
}

int main() {
    std::thread t1(enqueue);
    std::thread t3(enqueue);
    std::thread t2(dequeue);
    t1.join();
    t2.join();
    t3.join();
}
