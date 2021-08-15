#pragma once

#define NAME "/shmem-example"
#define NUM 3

#include <stdatomic.h>
struct Data {
  atomic_int state;
  int data[];
};

#define SIZE (sizeof(struct Data) + NUM * sizeof(int))
