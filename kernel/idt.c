#include "process.h"
#include "../include/types.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1
#define PIT_CH0   0x40
#define PIT_CMD   0x43
#define PIT_DIVISOR (1193182 / 100)

typedef struct {
    uint16_t low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t high;
} __attribute__((packed)) idt_gate_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_gate_t idt[256];
extern void timer_irq_stub(void);

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

void idt_init(void) {
    for (int i = 0; i < 256; i++) {
        idt[i].low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].flags = 0;
        idt[i].high = 0;
    }

    uint32_t address = (uint32_t)timer_irq_stub;
    idt[32].low = (uint16_t)address;
    idt[32].selector = 0x08;
    idt[32].flags = 0x8E;
    idt[32].high = (uint16_t)(address >> 16);

    outb(PIC1_CMD, 0x11); outb(PIC2_CMD, 0x11);
    outb(PIC1_DATA, 0x20); outb(PIC2_DATA, 0x28);
    outb(PIC1_DATA, 0x04); outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01); outb(PIC2_DATA, 0x01);
    outb(PIC1_DATA, 0xFE); outb(PIC2_DATA, 0xFF);

    outb(PIT_CMD, 0x36);
    outb(PIT_CH0, (uint8_t)(PIT_DIVISOR & 0xFF));
    outb(PIT_CH0, (uint8_t)(PIT_DIVISOR >> 8));

    idt_ptr_t pointer = {(uint16_t)(sizeof(idt) - 1), (uint32_t)idt};
    __asm__ __volatile__("lidtl %0" : : "m"(pointer));
    __asm__ __volatile__("sti");
}