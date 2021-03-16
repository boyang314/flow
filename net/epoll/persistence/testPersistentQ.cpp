#include "persistentQ.H"
#include <unistd.h>

int main() {
    persistentQ pq;
    pq.open();
    for(int i=0; i<1000; ++i) {
        MessageHeader msg;
        pq.write(msg);
        sleep(1);
    }
    pq.close();
}

