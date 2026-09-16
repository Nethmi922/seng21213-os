#ifndef PMM_H
#define PMM_H

#include "../include/types.h"

#define FRAME_SIZE 4096
#define PMM_LIMIT (32 * 1024 * 1024)

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) e820_entry_t;

void pmm_init(void);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t physical_address);
uint32_t pmm_free_frames(void);
uint32_t pmm_total_frames(void);

#endif