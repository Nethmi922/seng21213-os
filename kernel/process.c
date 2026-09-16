#include "process.h"
#include "vga.h"

#define PROCESS_STACK_TOP 0x800000

static pcb_t *ready_queue = 0;
static pcb_t *process_table[MAX_PROCESSES];
static uint32_t next_pid = 1;

static void process_set_default(pcb_t *p, uint32_t pid) {
    p->pid = pid;
    p->state = READY;
    p->esp = (uint32_t)&p->stack[STACK_SIZE / 4 - 1];
    p->eip = 0;
    p->next = 0;
}

static void enqueue(pcb_t *p) {
    if (!ready_queue) {
        ready_queue = p;
        p->next = 0;
        return;
    }

    pcb_t *cur = ready_queue;
    while (cur->next) cur = cur->next;
    cur->next = p;
    p->next = 0;
}

void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) process_table[i] = 0;
    ready_queue = 0;
    next_pid = 1;
}

pcb_t *process_create(void (*entry)(void)) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i] == 0) {
            pcb_t *p = (pcb_t *)((uint32_t)PROCESS_STACK_TOP -
                (i + 1) * (sizeof(pcb_t) + 0x100));
            process_table[i] = p;
            process_set_default(p, next_pid++);
            p->esp = (uint32_t)(&p->stack[STACK_SIZE / 4 - 1]);
            p->eip = (uint32_t)entry;
            enqueue(p);
            return p;
        }
    }
    return 0;
}

void process_dump(void) {
    vga_puts_color("\n  Process table\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  PID   STATE\n");
    pcb_t *cur = ready_queue;
    while (cur) {
        const char *state = "?";
        if (cur->state == READY) state = "READY";
        else if (cur->state == RUNNING) state = "RUNNING";
        else if (cur->state == BLOCKED) state = "BLOCKED";
        else if (cur->state == TERMINATED) state = "TERMINATED";
        vga_printf("  %u     %s\n", cur->pid, state);
        cur = cur->next;
    }
}

void process_kill(uint32_t pid) {
    pcb_t *cur = ready_queue;
    pcb_t *prev = 0;
    while (cur) {
        if (cur->pid == pid) {
            cur->state = TERMINATED;
            if (prev) prev->next = cur->next;
            else ready_queue = cur->next;
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

void process_yield(void) {
    if (!ready_queue) return;
    pcb_t *first = ready_queue;
    ready_queue = ready_queue->next;
    first->next = 0;
    enqueue(first);
}

void scheduler_tick(void) {
    process_yield();
}
