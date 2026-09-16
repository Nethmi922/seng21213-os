#include "semaphore.h"

static inline void interrupts_disable(void) {
    __asm__ __volatile__("cli" ::: "memory");
}

static inline void interrupts_enable(void) {
    __asm__ __volatile__("sti" ::: "memory");
}

void sem_init(semaphore_t *semaphore, int initial) {
    semaphore->count = initial;
    semaphore->nwaiters = 0;
}

void sem_wait(semaphore_t *semaphore) {
    for (;;) {
        interrupts_disable();
        if (semaphore->count > 0) {
            semaphore->count--;
            interrupts_enable();
            return;
        }
        if (semaphore->nwaiters < MAX_PROCS)
            semaphore->waiters[semaphore->nwaiters++] = current_proc;
        proc_table[current_proc].state = PROC_BLOCKED;
        interrupts_enable();
        __asm__ __volatile__("hlt");
    }
}

void sem_signal(semaphore_t *semaphore) {
    interrupts_disable();
    semaphore->count++;
    if (semaphore->nwaiters > 0) {
        int process = semaphore->waiters[--semaphore->nwaiters];
        if (proc_table[process].state == PROC_BLOCKED)
            proc_table[process].state = PROC_READY;
    }
    interrupts_enable();
}