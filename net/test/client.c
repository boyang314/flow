#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
   
#include "messageDefinition.h"

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        printf("%s ip port\n", argv[0]);
        return -1;
    }

    int sock = 0;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket creation error");
        return -1;
    }
   
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connection failed");
        return -1;
    }
    
    printf("Connected to %s:%d\n", argv[1], atoi(argv[2]));

    header_t h;
    h.magic = MAGIC;
    h.reqtype = ECHO;

    //h.reqtype = ECHO;
    char msg[27] = "abcdefghijklmnopqrstuvwxyz";

    //char buffer[BUFLEN] = {0};
    char newline = '\n';

    for(int count=0; count<10000; ++count) {
        for(int i = 10; i < sizeof(msg); ++i ) {
            h.len = sizeof(header_t) + i + 1;
            printf("len: %d\n", h.len);

            write(sock, &h, sizeof(h));
            write(sock, msg, i/2);
            //sleep(1);
            write(sock, msg + i/2, i - i/2);
            write(sock, &newline, 1);
            //sleep(1);
        }
        //usleep(1000); //too fast
        if (count%5==0) usleep(10000);
        //usleep(100000);
        //sleep(1);
    }

    close(sock);
    return 0;
}

