#pragma once

#define MAGIC 1234
#define BUFLEN 1024

typedef enum {
    ECHO = 0,
    CMD = 1,
} reqtype_t;

typedef struct header {
    int magic;
    int len;
    //int reqtype;
} header_t;

