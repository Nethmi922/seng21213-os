#include "thread.h"

tcb_t thread_table[MAX_THREADS];
int current_tid = 0;
static uint32_t next_tid = 1;

void thread_init(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_table[i].tid = 0;
        thread_table[i].pid = 0;
        thread_table[i].process_index = 0;
        thread_table[i].state = PROC_UNUSED;
        thread_table[i].name[0] = '\0';
    }
    current_tid = 0;
    next_tid = 1;
}

tcb_t *thread_create(uint32_t pid, const char *name, void (*fn)(void)) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].state != PROC_UNUSED) continue;
        pcb_t *process = proc_create(name, fn);
        if (!process) return 0;

        tcb_t *thread = &thread_table[i];
        thread->tid = next_tid++;
        thread->pid = pid;
        thread->process_index = (uint32_t)(process - proc_table);
        thread->state = PROC_READY;
        int j = 0;
        while (j < 23 && name[j]) {
            thread->name[j] = name[j];
            j++;
        }
        thread->name[j] = '\0';
        return thread;
    }
    return 0;
}

void thread_exit(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].process_index == (uint32_t)current_proc &&
            thread_table[i].state != PROC_UNUSED) {
            current_tid = i;
            thread_table[i].state = PROC_ZOMBIE;
            break;
        }
    }
    proc_exit();
}