#include "ramdisk.h"

static uint8_t ramdisk[RAMDISK_SIZE];

void ramdisk_init(void) {
    for (uint32_t i = 0; i < RAMDISK_SIZE; i++) ramdisk[i] = 0;
}

int ramdisk_read(uint32_t block, void *buffer) {
    if (block >= RAMDISK_BLOCKS || !buffer) return -1;
    uint8_t *destination = (uint8_t *)buffer;
    uint32_t offset = block * RAMDISK_BLOCK_SIZE;
    for (uint32_t i = 0; i < RAMDISK_BLOCK_SIZE; i++)
        destination[i] = ramdisk[offset + i];
    return 0;
}

int ramdisk_write(uint32_t block, const void *buffer) {
    if (block >= RAMDISK_BLOCKS || !buffer) return -1;
    const uint8_t *source = (const uint8_t *)buffer;
    uint32_t offset = block * RAMDISK_BLOCK_SIZE;
    for (uint32_t i = 0; i < RAMDISK_BLOCK_SIZE; i++)
        ramdisk[offset + i] = source[i];
    return 0;
}