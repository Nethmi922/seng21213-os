[BITS 32]

global timer_irq_stub
extern scheduler_tick

timer_irq_stub:
    pusha
    push esp
    call scheduler_tick
    add esp, 4
    mov esp, eax
    popa
    iretd