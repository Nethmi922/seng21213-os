#include "fs.h"
#include "ramdisk.h"

#define INODE_TABLE_BLOCK 3
#define INODE_TABLE_BLOCKS ((MAX_INODES * sizeof(inode_t)) / BLOCK_SIZE)
#define DATA_BLOCK_START (INODE_TABLE_BLOCK + INODE_TABLE_BLOCKS)

typedef struct {
    int used;
    int inode;
    int offset;
    int flags;
} file_t;

static superblock_t superblock;
static uint8_t inode_bitmap[MAX_INODES / 8];
static uint8_t block_bitmap[RAMDISK_BLOCKS / 8];
static file_t open_files[MAX_OPEN_FILES];

static int text_equal(const char *left, const char *right) {
    int i = 0;
    while (left[i] && right[i] && left[i] == right[i]) i++;
    return left[i] == right[i];
}

static int text_length(const char *text) {
    int length = 0;
    while (text[length]) length++;
    return length;
}

static void clear_bytes(void *memory, uint32_t count) {
    uint8_t *bytes = (uint8_t *)memory;
    for (uint32_t i = 0; i < count; i++) bytes[i] = 0;
}

static int bit_test(const uint8_t *bitmap, int bit) {
    return (bitmap[bit / 8] >> (bit % 8)) & 1;
}

static void bit_set(uint8_t *bitmap, int bit) {
    bitmap[bit / 8] |= (uint8_t)(1u << (bit % 8));
}

static void bit_clear(uint8_t *bitmap, int bit) {
    bitmap[bit / 8] &= (uint8_t)~(1u << (bit % 8));
}

static int inode_block(int index) {
    return INODE_TABLE_BLOCK + (index * (int)sizeof(inode_t)) / BLOCK_SIZE;
}

static int inode_offset(int index) {
    return (index * (int)sizeof(inode_t)) % BLOCK_SIZE;
}

static int inode_load(int index, inode_t *inode) {
    uint8_t block[BLOCK_SIZE];
    if (index < 0 || index >= MAX_INODES || ramdisk_read(inode_block(index), block) < 0)
        return -1;
    inode_t *stored = (inode_t *)(block + inode_offset(index));
    *inode = *stored;
    return 0;
}

static int inode_store(int index, const inode_t *inode) {
    uint8_t block[BLOCK_SIZE];
    if (index < 0 || index >= MAX_INODES || ramdisk_read(inode_block(index), block) < 0)
        return -1;
    inode_t *stored = (inode_t *)(block + inode_offset(index));
    *stored = *inode;
    return ramdisk_write(inode_block(index), block);
}

static int find_inode(const char *name) {
    inode_t inode;
    for (int i = 0; i < MAX_INODES; i++) {
        if (!bit_test(inode_bitmap, i)) continue;
        if (inode_load(i, &inode) == 0 && text_equal(inode.name, name)) return i;
    }
    return -1;
}

static int allocate_inode(void) {
    for (int i = 0; i < MAX_INODES; i++) {
        if (!bit_test(inode_bitmap, i)) {
            bit_set(inode_bitmap, i);
            superblock.free_inodes--;
            return i;
        }
    }
    return -1;
}

static int allocate_block(void) {
    for (int block = DATA_BLOCK_START; block < RAMDISK_BLOCKS; block++) {
        if (!bit_test(block_bitmap, block)) {
            bit_set(block_bitmap, block);
            superblock.free_blocks--;
            return block;
        }
    }
    return -1;
}

static void release_block(int block) {
    if (block >= (int)DATA_BLOCK_START && bit_test(block_bitmap, block)) {
        bit_clear(block_bitmap, block);
        superblock.free_blocks++;
    }
}

void fs_init(void) {
    ramdisk_init();
    clear_bytes(inode_bitmap, sizeof(inode_bitmap));
    clear_bytes(block_bitmap, sizeof(block_bitmap));
    clear_bytes(open_files, sizeof(open_files));
    clear_bytes(&superblock, sizeof(superblock));
    superblock.magic = FS_MAGIC;
    superblock.total_blocks = RAMDISK_BLOCKS;
    superblock.total_inodes = MAX_INODES;
    superblock.inode_table_off = INODE_TABLE_BLOCK * BLOCK_SIZE;
    superblock.data_off = DATA_BLOCK_START * BLOCK_SIZE;
    superblock.free_blocks = RAMDISK_BLOCKS - DATA_BLOCK_START;
    superblock.free_inodes = MAX_INODES;
    for (int block = 0; block < (int)DATA_BLOCK_START; block++) bit_set(block_bitmap, block);
}

int fs_open(const char *name, int flags) {
    if (!name || text_length(name) == 0 || text_length(name) >= 28) return -1;
    int inode_index = find_inode(name);
    if (inode_index < 0 && (flags & O_CREAT)) {
        inode_index = allocate_inode();
        if (inode_index < 0) return -1;
        inode_t inode;
        clear_bytes(&inode, sizeof(inode));
        inode.type = 1;
        int i = 0;
        while (name[i]) { inode.name[i] = name[i]; i++; }
        if (inode_store(inode_index, &inode) < 0) return -1;
    }
    if (inode_index < 0) return -1;
    if (flags & O_TRUNC) {
        inode_t inode;
        inode_load(inode_index, &inode);
        for (uint32_t i = 0; i < inode.block_count; i++) release_block(inode.blocks[i]);
        inode.size = 0;
        inode.block_count = 0;
        clear_bytes(inode.blocks, sizeof(inode.blocks));
        inode_store(inode_index, &inode);
    }
    for (int fd = 0; fd < MAX_OPEN_FILES; fd++) {
        if (!open_files[fd].used) {
            open_files[fd].used = 1;
            open_files[fd].inode = inode_index;
            open_files[fd].offset = 0;
            open_files[fd].flags = flags;
            return fd;
        }
    }
    return -1;
}

int fs_read(int fd, void *buffer, int count) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used || !buffer || count < 0)
        return -1;
    inode_t inode;
    if (inode_load(open_files[fd].inode, &inode) < 0) return -1;
    int remaining = (int)inode.size - open_files[fd].offset;
    if (remaining <= 0) return 0;
    if (count > remaining) count = remaining;
    uint8_t block[BLOCK_SIZE];
    uint8_t *output = (uint8_t *)buffer;
    int copied = 0;
    while (copied < count) {
        int block_index = open_files[fd].offset / BLOCK_SIZE;
        int within = open_files[fd].offset % BLOCK_SIZE;
        if (block_index >= (int)inode.block_count || ramdisk_read(inode.blocks[block_index], block) < 0) break;
        int amount = BLOCK_SIZE - within;
        if (amount > count - copied) amount = count - copied;
        for (int i = 0; i < amount; i++) output[copied + i] = block[within + i];
        copied += amount;
        open_files[fd].offset += amount;
    }
    return copied;
}

int fs_write(int fd, const void *buffer, int count) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used || !buffer || count < 0)
        return -1;
    inode_t inode;
    if (inode_load(open_files[fd].inode, &inode) < 0) return -1;
    const uint8_t *input = (const uint8_t *)buffer;
    int written = 0;
    while (written < count) {
        int block_index = open_files[fd].offset / BLOCK_SIZE;
        int within = open_files[fd].offset % BLOCK_SIZE;
        if (block_index >= INODE_DIRECT) break;
        if (block_index >= (int)inode.block_count) {
            int new_block = allocate_block();
            if (new_block < 0) break;
            inode.blocks[inode.block_count++] = new_block;
            uint8_t empty[BLOCK_SIZE];
            clear_bytes(empty, sizeof(empty));
            ramdisk_write(new_block, empty);
        }
        uint8_t block[BLOCK_SIZE];
        ramdisk_read(inode.blocks[block_index], block);
        int amount = BLOCK_SIZE - within;
        if (amount > count - written) amount = count - written;
        for (int i = 0; i < amount; i++) block[within + i] = input[written + i];
        ramdisk_write(inode.blocks[block_index], block);
        written += amount;
        open_files[fd].offset += amount;
        if ((uint32_t)open_files[fd].offset > inode.size)
            inode.size = (uint32_t)open_files[fd].offset;
    }
    inode_store(open_files[fd].inode, &inode);
    return written;
}

void fs_close(int fd) {
    if (fd >= 0 && fd < MAX_OPEN_FILES) open_files[fd].used = 0;
}

int fs_unlink(const char *name) {
    int inode_index = find_inode(name);
    if (inode_index < 0) return -1;
    inode_t inode;
    if (inode_load(inode_index, &inode) < 0) return -1;
    for (uint32_t i = 0; i < inode.block_count; i++) release_block(inode.blocks[i]);
    bit_clear(inode_bitmap, inode_index);
    superblock.free_inodes++;
    clear_bytes(&inode, sizeof(inode));
    return inode_store(inode_index, &inode);
}

int fs_ls(inode_t *out, int max) {
    int count = 0;
    inode_t inode;
    for (int i = 0; i < MAX_INODES && count < max; i++) {
        if (bit_test(inode_bitmap, i) && inode_load(i, &inode) == 0)
            out[count++] = inode;
    }
    return count;
}