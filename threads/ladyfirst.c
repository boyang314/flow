#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void* woman_dinner(void *data);
void* man_dinner(void *data);

pthread_mutex_t food_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t men_can_go_now_cond = PTHREAD_COND_INITIALIZER;
int thread_ids[3] = {0,1,2};

int
main(int argc, char *argv[])
{
  pthread_t threads[3];

  pthread_create(&threads[1], NULL,   man_dinner, &thread_ids[1]);
  pthread_create(&threads[2], NULL,   man_dinner, &thread_ids[2]);

  // a condition so that by the time the woman thread starts
  // the men threads have already started and are waiting
  // on the condition
  sleep(1);
  pthread_create(&threads[0], NULL, woman_dinner, &thread_ids[0]);

  for(int i=0;i<3;i++)
  {
    pthread_join(threads[i], NULL);
    printf("thread %i joined\n",i);
  }

  return 0;
}

void* woman_dinner(void *data)
{
  int *id = (int *) data;

  printf("woman %d - letting the men eat - waiting on food mutex\n",*id);
  pthread_mutex_lock(&food_mutex);
  printf("woman %d - letting the men eat - got food mutex - broadcast\n",*id);
  pthread_cond_broadcast(&men_can_go_now_cond);
  printf("woman %d - unlocking mutex\n",*id);
  pthread_mutex_unlock(&food_mutex);
  printf("woman %d - unlocked mutex\n",*id);

  return NULL;
}

void* man_dinner(void *data)
{
  int *id = (int *) data;

  printf("man %d waiting on a woman - waiting on food mutex\n",*id);
  pthread_mutex_lock(&food_mutex);
  printf("man %d waiting on a woman - got food mutex - waiting on condition\n",*id);
  pthread_cond_wait(&men_can_go_now_cond,&food_mutex);
  printf("man %d eating - got condition - unlocking mutex\n",*id);
  pthread_mutex_unlock(&food_mutex);
  printf("man %d eating after the lady - mutex unlocked\n",*id);

  return NULL;
}
