#ifndef _FS_H
#define _FS_H

// size in bytes of a 4kB FS block
#define FS_BLOCK_SIZE 4096
// size in bytes of the boot block or directory entry
#define FS_METADATA_SEGMENT_SIZE 64

#define MAX_NUM_DENTRIES 63

#include "types.h"
#include "syscall.h"
#include "scheduler.h"

/* boot block */
typedef struct boot_block_t {
		uint32_t num_dentries; // The number of directory entries after this boot block
		uint32_t num_inodes; // The number of 4kB index nodes in the file system
		uint32_t num_data_blocks; // The number of raw data blocks in the file system
		uint32_t reserved[13]; // 52 bytes of reserved values, aligns the boot block with 64 bytes
} boot_block_t;

/* directory entry */
typedef struct dentry_t {
		char file_name[32]; // The name of the file, up to 32 characters (not zero terminated)
		uint32_t file_type; // The type of file: 0 for RTC, 1 for file, 2 for directory
		uint32_t inode_index; // The index of the inode that corresponds to this directory
		uint32_t reserved[6]; // 24 bytes of reserved values, aligns the directory entries with 64 bytes
} dentry_t;

/* data block entry, makes indexing more sane */
typedef struct data_block_t {
		uint8_t data[FS_BLOCK_SIZE]; // Split into 4096 chars to read one byte at a time
} data_block_t;

/* inode entry */
typedef struct inode_block_t {
		uint32_t length; // Length of this inode in bytes
		uint32_t data_index[1023]; // Indexes of the up to 1023 data blocks this inode contains
} inode_block_t;

/* pointers to important sections */
boot_block_t* boot_block; // Pointer to the boot block
dentry_t* dentries; // Array of directory entries
inode_block_t* inodes; // Array of index node pointers
data_block_t* data_blocks; // Array of data block pointers

/* accessible functions */
/* Initializes above pointers, call this first. */
void init_fs(uint32_t fs_base_address);
/* Updates an elsewhere-allocated dentry with the values of one looked up by the file name. */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
/* Updates an elsewhere-allocated dentry with the values of the dentry at the provided index.
 * Much faster than reading by name, use this if you already have the index. */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
/* Reads data from the inode at the specified index, starting at offset and reading
 * length bytes.  The data goes into the buffer, make sure buf is large enough. */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
/* Open the file */
int32_t file_open(const uint8_t * filename);
/* Close the file */
int32_t file_close(int32_t fd);
/* Write to the file */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
/* Reads from the file */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
/* Reads from the directory (ls) */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);



#endif /* _FS_H */
