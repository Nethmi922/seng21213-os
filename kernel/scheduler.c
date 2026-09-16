#include "process.h"

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint32_t scheduler_tick(uint32_t *saved_esp) {
    proc_table[current_proc].esp = (uint32_t)saved_esp;
    proc_table[current_proc].ticks++;

    int next = (current_proc + 1) % MAX_PROCS;
    int searched = 0;
    while (searched < MAX_PROCS && proc_table[next].state != PROC_READY) {
        next = (next + 1) % MAX_PROCS;
        searched++;
    }
    if (searched == MAX_PROCS) {
        outb(0x20, 0x20);
        return (uint32_t)saved_esp;
    }

    proc_table[current_proc].state = PROC_READY;
    proc_table[next].state = PROC_RUNNING;
    current_proc = next;
    outb(0x20, 0x20);
    return proc_table[next].esp;
}