#ifndef PROCESS_H
#define PROCESS_H

#include "../include/types.h"

#define MAX_PROCESSES    16
#define STACK_SIZE       4096

typedef enum {
    READY = 0,
    RUNNING = 1,
    BLOCKED = 2,
    TERMINATED = 3
} proc_state_t;

typedef struct pcb {
    uint32_t pid;
    proc_state_t state;
    uint32_t esp;
    uint32_t eip;
    uint32_t stack[STACK_SIZE / 4];
    struct pcb *next;
} pcb_t;

void process_init(void);
pcb_t *process_create(void (*entry)(void));
void process_dump(void);
void process_kill(uint32_t pid);
void process_yield(void);
void scheduler_tick(void);

#endif /* PROCESS_H */
