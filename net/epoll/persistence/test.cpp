#include "persistedBuffer.H"
#include <iostream>

int main() {
    persistedBuffer pBuf("tPersistence", 1024);
    char message[] = "hello world!";
    for (int i=0; i<1024; ++i)
        pBuf.add(message, sizeof(message));

    std::cout << pBuf.getSize() << ":" << pBuf.getNumOfMsgs() << '\n';
    //pBuf.clean();
    return 0;
}



