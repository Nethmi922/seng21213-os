#include "pmm.h"
#include "ramdisk.h"

#define MAX_FRAMES (PMM_LIMIT / FRAME_SIZE)
#define BITMAP_WORDS (MAX_FRAMES / 32)
#define E820_COUNT ((volatile uint16_t *)0x8000)
#define E820_MAP ((volatile e820_entry_t *)0x8004)
#define KERNEL_END 0x140000

static uint32_t frame_bitmap[BITMAP_WORDS];
static uint32_t free_frame_count;

static inline void frame_set(uint32_t frame) {
    frame_bitmap[frame / 32] |= 1u << (frame % 32);
}

static inline void frame_clear(uint32_t frame) {
    frame_bitmap[frame / 32] &= ~(1u << (frame % 32));
}

static inline int frame_used(uint32_t frame) {
    return (frame_bitmap[frame / 32] >> (frame % 32)) & 1u;
}

void pmm_init(void) {
    for (uint32_t i = 0; i < BITMAP_WORDS; i++) frame_bitmap[i] = 0xFFFFFFFFu;
    free_frame_count = 0;

    uint16_t count = *E820_COUNT;
    for (uint16_t i = 0; i < count; i++) {
        e820_entry_t entry = E820_MAP[i];
        if (entry.type != 1 || entry.base >= PMM_LIMIT) continue;

        uint64_t region_end = entry.base + entry.length;
        if (region_end > PMM_LIMIT) region_end = PMM_LIMIT;
        uint64_t first = entry.base < 0x100000 ? 0x100000 : entry.base;
        if (first < KERNEL_END) first = KERNEL_END;
        first = (first + FRAME_SIZE - 1) & ~(uint64_t)(FRAME_SIZE - 1);
        region_end &= ~(uint64_t)(FRAME_SIZE - 1);

        for (uint64_t address = first; address < region_end; address += FRAME_SIZE) {
            uint32_t frame = (uint32_t)(address / FRAME_SIZE);
            if (address >= RAMDISK_BASE && address < RAMDISK_BASE + RAMDISK_SIZE)
                continue;
            if (frame_used(frame)) {
                frame_clear(frame);
                free_frame_count++;
            }
        }
    }
}

uint32_t pmm_alloc_frame(void) {
    for (uint32_t frame = 0; frame < MAX_FRAMES; frame++) {
        if (!frame_used(frame)) {
            frame_set(frame);
            free_frame_count--;
            return frame * FRAME_SIZE;
        }
    }
    return 0;
}

void pmm_free_frame(uint32_t physical_address) {
    uint32_t frame = physical_address / FRAME_SIZE;
    if (frame >= MAX_FRAMES || !frame_used(frame)) return;
    frame_clear(frame);
    free_frame_count++;
}

uint32_t pmm_free_frames(void) {
    return free_frame_count;
}

uint32_t pmm_total_frames(void) {
    return MAX_FRAMES;
}