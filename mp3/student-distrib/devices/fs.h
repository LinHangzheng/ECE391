/* fs.h - Implement for file system */
/* Created by Chenting on 10/22/2019 */

#ifndef _FS_H
#define _FS_H

#include "../types.h"
#include "../lib.h"

/* constant used in file system */
#define MAX_FILE_NUM        63
#define BLOCK_SIZE_IN_BYTE  4096
#define INODE_MAX_DATA_BLOCK   ((BLOCK_SIZE_IN_BYTE / 4) - 1)
#define MAX_FILENAME_LEN    32
#define RESERVED_LEN_D       24 
#define RESERVED_LEN_B       52 

/* structure used in file system */
typedef struct dentry_t{
    char filename[MAX_FILENAME_LEN];
    uint32_t file_type;
    uint32_t inode_idx;
    uint8_t reserved[RESERVED_LEN_D];
} dentry_t;

/* structure used in file system */
typedef struct boot_block_t{
    uint32_t num_dentries;
    uint32_t num_inodes;
    uint32_t num_data_blocks;
    uint8_t reserved[RESERVED_LEN_B];
    dentry_t dentry_array[MAX_FILE_NUM];  // bug log
} boot_block_t;

/* structure used in file system */
typedef struct inode_t{
    uint32_t file_size;
    uint32_t data_blocks[INODE_MAX_DATA_BLOCK];
} inode_t;  

/* structure used in file system */
typedef struct data_block_t{
    uint8_t datas[BLOCK_SIZE_IN_BYTE];
} data_block_t;

/* ini file sys */
int32_t fs_init(uint32_t fs_start_ptr);
/* search the given bit and find dentry structrue whose name is fname */
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry);
/* search dentry by index */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
/* read data from specific place and copy to the given pointer */
int32_t read_data (uint32_t inode_idx, uint32_t offset, uint8_t* buf, uint32_t length);


/* open the data file in the directory (not implemented yet) */
int32_t file_open (const uint8_t* filename);
/* close the data file in the directory (not implemented yet) */
int32_t file_close (int32_t fd);
/* read_dentry_by_index */
int32_t file_read (int32_t fd, uint32_t offset, void* buf, int32_t nbytes);
 /* return -1 nothing, read only*/
int32_t file_write (int32_t fd, const void* buf, int32_t nbytes);


/* open the data dir in the directory (not implemented yet) */
int32_t dir_open (const uint8_t* filename);
/* close the data dir in the directory (not implemented yet) */
int32_t dir_close (int32_t fd);
/* read_dentry_by_index */
int32_t dir_read (int32_t fd,  uint32_t offset, void* input_buf, int32_t nbytes);
 /* return -1 nothing, read only*/
int32_t dir_write (int32_t fd,  const void* buf, int32_t nbytes);

/* helper in test, just print the content in the input dentry */
void print_dentry(dentry_t dentry);
/* helper in test, just print the content in the input datablock */
void print_data_block(data_block_t data_block);
/* helper in test, return the boot block */
boot_block_t get_boot_block(void);
/*  helper function used to get size of corresponding inode */
uint32_t get_file_size(uint32_t inode_num);

#endif
