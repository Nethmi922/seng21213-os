#include "process.h"
#include "vga.h"

pcb_t proc_table[MAX_PROCS];
int current_proc = 0;
static uint8_t process_stacks[MAX_PROCS][STACK_SIZE];
static uint32_t next_pid = 1;

void process_init(void) {
    for (int i = 0; i < MAX_PROCS; i++) {
        proc_table[i].pid = 0;
        proc_table[i].state = PROC_UNUSED;
        proc_table[i].esp = 0;
        proc_table[i].stack_base = (uint32_t)process_stacks[i];
        proc_table[i].entry = 0;
        proc_table[i].name[0] = '\0';
        proc_table[i].ticks = 0;
    }
    current_proc = 0;
    next_pid = 1;
}

pcb_t *proc_create(const char *name, void (*entry)(void)) {
    for (int i = 0; i < MAX_PROCS; i++) {
        if (proc_table[i].state == PROC_UNUSED) {
            pcb_t *p = &proc_table[i];
            uint32_t *sp = (uint32_t *)(p->stack_base + STACK_SIZE);

            /* timer_irq_stub restores this layout with POPA/IRETD. */
            *--sp = 0x202;
            *--sp = 0x08;
            *--sp = (uint32_t)entry;
            for (int r = 0; r < 8; r++) *--sp = 0;

            p->pid = next_pid++;
            p->state = PROC_READY;
            p->esp = (uint32_t)sp;
            p->entry = entry;
            p->ticks = 0;
            int j = 0;
            while (j < 31 && name[j]) {
                p->name[j] = name[j];
                j++;
            }
            p->name[j] = '\0';
            return p;
        }
    }
    return 0;
}

void process_dump(void) {
    static const char *states[] = {"UNUSED", "READY", "RUNNING", "BLOCKED", "ZOMBIE"};
    vga_puts("\nPID NAME STATE TICKS\n");
    for (int i = 0; i < MAX_PROCS; i++) {
        if (proc_table[i].state == PROC_UNUSED) continue;
        vga_printf("%u %s %s %u\n", proc_table[i].pid,
                   proc_table[i].name, states[proc_table[i].state],
                   proc_table[i].ticks);
    }
}

void process_kill(uint32_t pid) {
    for (int i = 0; i < MAX_PROCS; i++) {
        if (proc_table[i].pid == pid && proc_table[i].state != PROC_UNUSED) {
            proc_table[i].state = PROC_ZOMBIE;
            return;
        }
    }
}

void proc_exit(void) {
    proc_table[current_proc].state = PROC_ZOMBIE;
    for (;;) __asm__ __volatile__("hlt");
}
