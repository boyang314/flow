#define _GNU_SOURCE //pthread_yield

#include <sched.h> //sched_yield, sched_setaffinity

#include <pthread.h>   
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> //gettid
#include <sys/syscall.h>

#define BUFLEN 1024
#define NUMTHR 4

FILE *logfile;

void* consumer(void *arg);
void* producer(void *arg);

char buffer[BUFLEN];
int buflen;

pthread_mutex_t bufmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t notEmpty  = PTHREAD_COND_INITIALIZER;
pthread_cond_t notFull  = PTHREAD_COND_INITIALIZER;

typedef struct {
    pid_t _tid;
	int _seqNo;
} Msg;

int main()
{
    logfile = fopen("./out.txt", "w");

	memset(buffer, 0, BUFLEN);
	buflen = 0;

    int policy = SCHED_FIFO; //SCHED_NORMAL;
    printf("max:%d,min:%d\n", sched_get_priority_max(policy), sched_get_priority_min(policy));

	pthread_t thread[NUMTHR];
	pthread_create(&thread[0], NULL, (void *)consumer, NULL);
	pthread_create(&thread[1], NULL, (void *)consumer, NULL);
	pthread_create(&thread[2], NULL, (void *)producer, NULL);
	pthread_create(&thread[3], NULL, (void *)producer, NULL);

	for(int i=0; i< NUMTHR; i++)
	{
		pthread_join(thread[i], NULL);
	}
}

void* consumer(void *arg) {
    //pid_t tid = gettid();
    pid_t tid = syscall(SYS_gettid);
    //sched_setscheduler(tid, SCHED_FIFO, NULL);
    //pthread_setschedprio(pthread_self(), 99);

	while(1) {
		pthread_mutex_lock(&bufmutex);
        while (buflen == 0) pthread_cond_wait(&notEmpty, &bufmutex);

		if (buflen >= sizeof(Msg)) {
			Msg *msg = (Msg*)buffer;
			fprintf(logfile, "%d consume %d %d\n", tid, msg->_tid, msg->_seqNo);
            fflush(logfile);
			buflen -= sizeof(Msg);
			memmove(buffer, buffer+sizeof(Msg), buflen);
		}

        pthread_cond_signal(&notFull);
		pthread_mutex_unlock(&bufmutex);
        sched_yield(); //meant for SCHED_RR & FIFO
	}
}

void* producer(void *arg) {
    //pid_t tid = gettid();
    pid_t tid = syscall(SYS_gettid);
    //sched_setscheduler(tid, SCHED_FIFO, NULL);
    //pthread_setschedprio(pthread_self(), 1);

	for(int i=0; i<100; ++i) {
		pthread_mutex_lock(&bufmutex);
        while (buflen > BUFLEN - sizeof(Msg)) pthread_cond_wait(&notFull, &bufmutex);

		Msg msg;
        msg._tid = tid;
		msg._seqNo = i;
		memcpy(buffer+buflen, &msg, sizeof(Msg));
		buflen += sizeof(Msg);
		fprintf(logfile, "%d produce %d\n", tid, msg._seqNo);
        fflush(logfile);

        pthread_cond_signal(&notEmpty);
		pthread_mutex_unlock(&bufmutex);
        //pthread_yield();
        sched_yield(); //meant for SCHED_RR & FIFO
	}
}
