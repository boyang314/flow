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
#define QLENMASK 255 //ring buffer

typedef struct {
    pthread_mutex_t bmutex;
    int sock, buflen;
    char buf[BUFLEN];
    int cleanup;
} peer_t;
peer_t peers[MAXSOCKS];

typedef struct {
    pthread_t worker;
    pthread_mutex_t qmutex;
    pthread_cond_t notEmpty;
    int workerid;
    int head, tail;
    peer_t* peers[QLEN];
} worker_t;
worker_t workers[NWORKERS]; //worker pool

void init_peer(peer_t *peer);
void init_peers();
void init_worker(worker_t *worker, int id);
void init_workers();

void *do_work(void *worker);
void enqueue_work(peer_t *peer);

int process_p(peer_t* p);

int on_connect(int sock);
int on_disconnect(int sock);
int on_data(int sock, char *buf, int len);

int main(int argc, char *argv[])
{
    int port = 5001;
    if (argc == 2) port = atoi(argv[1]);

    int sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock < 0) {
        perror("could not create socket\n");
        exit(1);
    }
    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *)&server , sizeof(server)) < 0) {
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
    while(1) {
        r_set = active_fd_set;
        if (select(socket_max + 1, &r_set, NULL, NULL, NULL) < 0) {
            perror("select");
            close(sock);
            exit(1);
        }

        for(int i = 0; i < socket_max + 1; ++i) {
            if (!FD_ISSET(i, &r_set)) continue;
            if (i == sock) { //server sock
                struct sockaddr_in client;
                int len = sizeof(struct sockaddr_in);
                int new_socket = accept(sock, (struct sockaddr *)&client, (socklen_t*)&len);
                if (new_socket<0) {
                    perror("accept failed\n");
                    continue; //exit(1); //should clean shutdown server
                }
                else if (new_socket>MAXSOCKS) {
                    printf("exceed max allowed number of sockets\n");
                    close(new_socket);
                } else {
                    FD_SET(new_socket, &active_fd_set);
                    if (new_socket > socket_max) socket_max = new_socket;
                    on_connect(new_socket);
                }
            } else {
                char buf[BUFLEN];
                int read_size = read(i, buf, BUFLEN);
                if (read_size <= 0) { //use async, 0 + EAGAIN should be fine
                    if (read_size < 0) perror("recv failed");
                    on_disconnect(i);
                    FD_CLR(i, &active_fd_set);
                } else {
                    on_data(i, buf, read_size);
                }
            }
        }
    }

    return 0;
}

//assume single thread, -1 for bad buffer
int process_p(peer_t* p) {
    //race condition on p->buflen
    while (p->buflen >= sizeof(header_t)) {
        header_t *hd = (header_t*)p->buf;
        //printf("sock: %d, buffer len: %d, message len:%d\n", p->sock, p->buflen, hd->len);

        // invalid header
        if(hd->magic != MAGIC || hd->len > BUFLEN) {
            printf("sock %d received invalid packet\n", p->sock);
            return -1;
        }

        // partial packet
        if(hd->len > p->buflen) {
            //printf("partial packet\n");
            return 0;
        }

        // complete message
        //printf("complete packet from peer %lu:%d\n", pthread_self(), p->sock);
        int strlen = hd->len - sizeof(header_t);
        //printf("sock:%d reqtype:%d payload:%*.*s\n", p->sock, hd->reqtype, strlen, strlen, p->buf + sizeof(header_t));
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
    int next = (w->tail+1) & QLENMASK;
    if (next == w->head) { //add cond var
        printf("queue full: id:%d head:%d tail:%d\n", w->workerid, w->head, w->tail);
        exit(1);
    }
    //printf("enqueue_work id:%d head:%d tail:%d\n", w->workerid, w->head, w->tail);
    w->peers[w->tail] = peer;
    w->tail = next;
    pthread_cond_signal(&(w->notEmpty));
    pthread_mutex_unlock(&(w->qmutex));
}

void* do_work(void *worker) {
    worker_t *w = (worker_t*)worker;
    while(1) {
        pthread_mutex_lock(&(w->qmutex));
        while (w->head == w->tail) 
            pthread_cond_wait(&(w->notEmpty), &(w->qmutex));
        {
            //printf("do_work id:%d head:%d tail:%d\n", w->workerid, w->head, w->tail);
            peer_t *peer = w->peers[w->head];
            int ret = process_p(peer);
            if (ret >= 0) {
                w->head = ++w->head & QLENMASK;
            } 
            else if (ret < 0) {
                exit(1);
            }
            //race condition peer->cleanup
            if (peer->cleanup) {
                close(peer->sock);
                init_peer(peer);
            }
        }
        pthread_mutex_unlock(&(w->qmutex));
    }
}

void init_worker(worker_t *worker, int id) {
    pthread_mutex_init(&(worker->qmutex), NULL);
    pthread_cond_init(&(worker->notEmpty), NULL);
    worker->workerid = id;
    worker->head = 0;
    worker->tail = 0;
    pthread_create(&(worker->worker), NULL, (void *)do_work, worker);
}

void init_workers() {
    for(int i=0; i<NWORKERS; ++i) {
        init_worker(&workers[i], i);
    }
}

void init_peer(peer_t *peer) {
    peer->sock = 0;
    peer->buflen = 0;
    pthread_mutex_init(&(peer->bmutex), NULL);
    memset(peer->buf, 0, BUFLEN);
    peer->cleanup = 0;
}

void init_peers() {
    for(int i=0; i<MAXSOCKS; ++i) {
        init_peer(&peers[i]);
    }
}

int on_connect(int sock) {
    //book keeping
    peers[sock].sock = sock; //check this is not existing active sock
    printf("client sock:%d connected\n", sock);
    return sock;
}

int on_disconnect(int sock) {
    //book keeping
    printf("client sock:%d disconnected, remaing buflen:%d\n", sock, peers[sock].buflen);
    peers[sock].cleanup = 1; //enqueue with cleanup=1
    enqueue_work(&(peers[sock]));
    /*
    close(sock); //premature if sock buffer not empty
    init_peer(sock);
    */
    return sock;
}

int on_data(int sock, char *buf, int len) {
    peer_t *p = &peers[sock];
    pthread_mutex_lock(&(p->bmutex));
    if (p->buflen + len < BUFLEN) {
        memcpy(p->buf+p->buflen, buf, len);
        p->buflen += len;
    } else {
        printf("sock:%d buffer full\n", sock);
    }
    pthread_mutex_unlock(&(p->bmutex));
    enqueue_work(p);
    return len;
} 
