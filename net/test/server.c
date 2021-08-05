#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>	
#include<pthread.h>

#include "messageDefinition.h"

typedef struct
{
    int sock, buflen;
    char buf[BUFLEN];
} peer_t;

int process_p(peer_t* p);
void *connection_handler(void *);

int main(int argc, char *argv[])
{
    int port = 5001;
    if (argc == 2) port = atoi(argv[1]);

    int sock = 0;
    if ((sock = socket(AF_INET , SOCK_STREAM , 0)) < 0)
    {
        perror("could not create socket\n");
        exit(1);
    }
    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("bind failed\n");
        exit(1);
    }

    listen(sock, 50);

    struct sockaddr_in client;
    int new_socket, *new_sock;
    int len = sizeof(struct sockaddr_in);
    while ((new_socket = accept(sock, (struct sockaddr *)&client, (socklen_t*)&len)) > 0)
    {
        pthread_t handler;
        new_sock = malloc(1);
        *new_sock = new_socket;

        if (pthread_create(&handler, NULL, connection_handler, (void*)new_sock) < 0)
        {
            perror("could not create thread");
            exit(1);
        }
    }

    if (new_socket<0)
    {
        perror("accept failed\n");
        exit(1);
    }

    return 0;
}

void *connection_handler(void *sockfd)
{
    peer_t p;
    p.sock = *(int*)sockfd;
    memset(p.buf, sizeof(p.buf), 0);
    p.buflen = 0;
	int read_size;
	while ((read_size = read(p.sock, p.buf+p.buflen, sizeof(p.buf)-p.buflen)) > 0)
	{
        p.buflen += read_size;
        int ret = process_p(&p);
        if (ret < 0) {
            printf("client socck:%d received invalid packet\n", p.sock);
            //kill the connection
        }
	}
	
	if (read_size == 0)
	{
		printf("client socck:%d disconnected\n", p.sock);
	}
	else if (read_size == -1)
	{
		perror("recv failed");
		printf("client socck:%d read failed\n", p.sock);
	}
		
	free(sockfd);
    return 0;
}

//assume single thread, -1 for bad buffer
int process_p(peer_t* p) {
    while (p->buflen >= sizeof(header_t)) {
        header_t *hd = (header_t*)p->buf;
        printf("sock: %d, buffer len: %d, message len:%d\n", p->sock, p->buflen, hd->len);

        // invalid header
        if(hd->magic != MAGIC || hd->len > BUFLEN)
        {
            printf("invalid packet\n");
            return -1;
        }

        // partial packet
        if(hd->len > p->buflen)
        {
            printf("partial packet\n");
            return 0;
        }

        // complete message
        printf("complete packet from peer %lu:%d\n", pthread_self(), p->sock);
        int strlen = hd->len - sizeof(header_t);
        printf("payload:%*.*s\n", strlen, strlen, p->buf + sizeof(header_t));
        //write(sock, message, strlen(message));

        p->buflen -= hd->len;
        if (p->buflen > 0) memmove(p->buf, p->buf + hd->len, p->buflen);
    }
}
