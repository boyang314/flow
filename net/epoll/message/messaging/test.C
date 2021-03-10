#include "test.H"
#include <iostream>

int main() {
    struct testMsg1 t1;
    struct testMsg2 t2;
    std::cout << sizeof(t1) << std::endl;
    std::cout << sizeof(t2) << std::endl;
}
