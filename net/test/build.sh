#serverr: thread per conn server
#server2: select based thread per conn
#server3: select based thread poll
gcc -o client client.c
gcc -o server server.c -lpthread
gcc -o server2 server2.c -lpthread
gcc -g -o server3 server3.c -lpthread
