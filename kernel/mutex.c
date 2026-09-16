#include "mutex.h"

static inline int atomic_xchg(volatile int *address, int value) {
    int old;
    __asm__ __volatile__("xchgl %0, %1"
        : "=r"(old), "+m"(*address) : "0"(value) : "memory");
    return old;
}

void mutex_init(mutex_t *mutex) {
    mutex->locked = 0;
    mutex->owner = -1;
    mutex->nwaiters = 0;
}

void mutex_lock(mutex_t *mutex) {
    while (atomic_xchg(&mutex->locked, 1) != 0) {
        if (mutex->nwaiters < MAX_PROCS)
            mutex->waiters[mutex->nwaiters++] = current_proc;
        proc_table[current_proc].state = PROC_BLOCKED;
        __asm__ __volatile__("hlt");
    }
    mutex->owner = current_proc;
}

void mutex_unlock(mutex_t *mutex) {
    mutex->owner = -1;
    mutex->locked = 0;
    if (mutex->nwaiters > 0) {
        int process = mutex->waiters[--mutex->nwaiters];
        if (proc_table[process].state == PROC_BLOCKED)
            proc_table[process].state = PROC_READY;
    }
}