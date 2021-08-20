gcc -o futex_demo futex_demo.c
gcc -o futex_demo2 futex_demo2.c
g++ -o notifier notifier.c -lpthread
g++ -o notifier2 notifier2.c -lpthread
g++ -o eventfd eventfd.c -lpthread
