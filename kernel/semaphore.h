#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "process.h"

typedef struct {
    volatile int count;
    int waiters[MAX_PROCS];
    int nwaiters;
} semaphore_t;

void sem_init(semaphore_t *semaphore, int initial);
void sem_wait(semaphore_t *semaphore);
void sem_signal(semaphore_t *semaphore);

#endif