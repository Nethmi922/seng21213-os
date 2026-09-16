#ifndef THREAD_H
#define THREAD_H

#include "process.h"

#define MAX_THREADS MAX_PROCS

typedef struct {
    uint32_t tid;
    uint32_t pid;
    uint32_t process_index;
    proc_state_t state;
    char name[24];
} tcb_t;

extern tcb_t thread_table[MAX_THREADS];
extern int current_tid;

void thread_init(void);
tcb_t *thread_create(uint32_t pid, const char *name, void (*fn)(void));
void thread_exit(void);

#endif