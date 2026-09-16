#ifndef MUTEX_H
#define MUTEX_H

#include "process.h"

typedef struct {
    volatile int locked;
    int owner;
    int waiters[MAX_PROCS];
    int nwaiters;
} mutex_t;

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

#endif