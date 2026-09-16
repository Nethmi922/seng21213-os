#ifndef RAMDISK_H
#define RAMDISK_H

#include "../include/types.h"

#define RAMDISK_SIZE (1024 * 1024)
#define RAMDISK_BASE 0x300000
#define RAMDISK_BLOCK_SIZE 512
#define RAMDISK_BLOCKS (RAMDISK_SIZE / RAMDISK_BLOCK_SIZE)

void ramdisk_init(void);
int ramdisk_read(uint32_t block, void *buffer);
int ramdisk_write(uint32_t block, const void *buffer);

#endif