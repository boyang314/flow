#include <pthread.h>   
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define BUFLEN 1024
#define NUMTHR 2

void* consumer(void *arg);
void* producer(void *arg);

char buffer[BUFLEN];
int buflen;

pthread_mutex_t bufmutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t nonEmpty  = PTHREAD_COND_INITIALIZER;
//pthread_cond_t full  = PTHREAD_COND_INITIALIZER;

typedef struct {
	int i_;
} Msg;

int main()
{
	memset(buffer, 0, BUFLEN);
	buflen = 0;

	pthread_t thread[NUMTHR];
	pthread_create(&thread[0], NULL, (void *)consumer, NULL);
	pthread_create(&thread[1], NULL, (void *)producer, NULL);

	for(int i=0; i< NUMTHR; i++)
	{
		pthread_join(thread[i], NULL);
	}
}

void* consumer(void *arg) {
	while(1) {
		pthread_mutex_lock(&bufmutex);
		if (buflen >= sizeof(Msg)) {
			Msg *msg = (Msg*)buffer;
			printf("consume %d\n", msg->i_);
			buflen -= sizeof(Msg);
			memmove(buffer, buffer+sizeof(Msg), buflen);
		}
		pthread_mutex_unlock(&bufmutex);
	}
}

void* producer(void *arg) {
	for(int i=0; i<10; ++i) {
		pthread_mutex_lock(&bufmutex);
		Msg msg;
		msg.i_ = i;
		memcpy(buffer+buflen, &msg, sizeof(Msg));
		buflen += sizeof(Msg);
		printf("produce %d\n", msg.i_);
		pthread_mutex_unlock(&bufmutex);
        sleep(1);
	}
}
