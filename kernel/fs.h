#ifndef FS_H
#define FS_H

#include "../include/types.h"

#define FS_MAGIC 0x021213F5
#define BLOCK_SIZE 512
#define MAX_INODES 1024
#define INODE_DIRECT 8
#define MAX_OPEN_FILES 16

#define O_RDONLY 0x01
#define O_WRONLY 0x02
#define O_CREAT  0x04
#define O_TRUNC  0x08

typedef struct {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t total_inodes;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t inode_table_off;
    uint32_t data_off;
    uint8_t reserved[484];
} __attribute__((packed)) superblock_t;

typedef struct {
    uint32_t size;
    uint32_t blocks[INODE_DIRECT];
    uint32_t block_count;
    uint8_t type;
    char name[28];
    uint8_t reserved[187];
} __attribute__((packed)) inode_t;

void fs_init(void);
int fs_open(const char *name, int flags);
int fs_read(int fd, void *buffer, int count);
int fs_write(int fd, const void *buffer, int count);
void fs_close(int fd);
int fs_unlink(const char *name);
int fs_ls(inode_t *out, int max);

#endif