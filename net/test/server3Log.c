//deliver work to a worker pool
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>	
#include<pthread.h>

#include "messageDefinition.h"

#define NWORKERS 3
#define MAXSOCKS 256
#define QLEN 256

typedef struct
{
    int sock, buflen;
    pthread_mutex_t bmutex;
    char buf[BUFLEN];
} peer_t;
peer_t peers[MAXSOCKS];

typedef struct 
{
    pthread_t worker;
    pthread_mutex_t qmutex;
    pthread_cond_t notEmpty;
    int workerid;
    int head, tail;
    peer_t* peers[QLEN];
} worker_t;
worker_t workers[NWORKERS]; //worker pool

void init_peers();
void init_workers();

void *do_work(void *worker);
void enqueue_work(peer_t *peer);

int process_p(peer_t* p);

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

    init_peers();
    init_workers();

    fd_set r_set, active_fd_set; 
    FD_ZERO(&r_set);
    FD_ZERO(&active_fd_set);
    FD_SET(sock, &active_fd_set);

    int socket_max = sock;
    while(1)
    {
        r_set = active_fd_set;
        if (select(socket_max + 1, &r_set, NULL, NULL, NULL) < 0)
        {
            perror("select");
            close(sock);
            exit(1);
        }

        for(int i = 0; i < socket_max + 1; ++i)
        {
            if (FD_ISSET (i, &r_set))
            {
                if (i == sock)
                {
                    struct sockaddr_in client;
                    int len = sizeof(struct sockaddr_in);
                    int new_socket;
                    if ((new_socket = accept(sock, (struct sockaddr *)&client, (socklen_t*)&len)) > 0)
                    {
                        FD_SET(new_socket, &active_fd_set);
                        if (new_socket > socket_max) socket_max = new_socket;
                    }
                    if (new_socket<0 || new_socket>MAXSOCKS)
                    {
                        perror("accept failed\n");
                        exit(1);
                    }
                    peers[new_socket].sock = new_socket;
                } 
                else
                {
                    peer_t *p = &peers[i];
                    pthread_mutex_lock(&(p->bmutex));
                    int read_size = read(p->sock, p->buf+p->buflen, sizeof(p->buf)-p->buflen);
                    if (read_size > 0)
                    {
                        p->buflen += read_size;
                        enqueue_work(p);
                    }
                    pthread_mutex_unlock(&(p->bmutex));

                    if (read_size == 0)
                    {
                        printf("client sock:%d disconnected\n", p->sock);
                        FD_CLR(i, &active_fd_set);
                    }
                    else if (read_size == -1)
                    {
                        perror("recv failed");
                        printf("client sock:%d read failed\n", p->sock);
                        FD_CLR(i, &active_fd_set);
                    }
                }
            }
        }
    }

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

        pthread_mutex_lock(&(p->bmutex));
        p->buflen -= hd->len;
        if (p->buflen > 0) memmove(p->buf, p->buf + hd->len, p->buflen);
        pthread_mutex_unlock(&(p->bmutex));
    }
    return 0;
}

void enqueue_work(peer_t *peer) {
    int worker_index = peer->sock % NWORKERS;
    worker_t *w = &workers[worker_index];

    pthread_mutex_lock(&(w->qmutex));
    printf("sockfd:%d equeue:%d\n", peer->sock, worker_index);
    printf("enqueue_work id:%d head:%d tail:%d\n", w->workerid, w->head, w->tail);
    w->peers[w->tail] = peer;
    w->tail = ++w->tail % QLEN;
    pthread_cond_signal(&(w->notEmpty));
    pthread_mutex_unlock(&(w->qmutex));
}

void* do_work(void *worker) {
    worker_t *w = (worker_t*)worker;
    while(1) 
    {
        pthread_mutex_lock(&(w->qmutex));
        if (w->head < w->tail) 
        {
            printf("do_work id:%d head:%d tail:%d\n", w->workerid, w->head, w->tail);
            int ret = process_p(w->peers[w->head]);
            if (ret >= 0)
            {
                w->head = ++w->head % QLEN;
            } 
            else if (ret < 0) {
                printf("received invalid packet\n");
                exit(1);
            }
        }
        printf("right before cond wait\n");
        pthread_cond_wait(&(w->notEmpty), &(w->qmutex));
        pthread_mutex_unlock(&(w->qmutex));
    }
}

void init_worker(worker_t *worker, int id) 
{
    pthread_mutex_init(&(worker->qmutex), NULL);
    pthread_cond_init(&(worker->notEmpty), NULL);
    worker->workerid = id;
    worker->head = 0;
    worker->tail = 0;
    pthread_create(&(worker->worker), NULL, (void *)do_work, worker);
}

void init_workers() {
    for(int i=0; i<NWORKERS; ++i) 
    {
        init_worker(&workers[i], i);
    }
}

void init_peers() {
    for(int i=0; i<MAXSOCKS; ++i) 
    {
        peers[i].sock = 0;
        peers[i].buflen = 0;
        pthread_mutex_init(&(peers[i].bmutex), NULL);
        memset(peers[i].buf, 0, BUFLEN);
    }
}
