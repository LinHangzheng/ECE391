#ifndef _DO_SYS_CALL_H
#define _DO_SYS_CALL_H

#include  "../fop.h"
#include  "../devices/fs.h"
#include  "../types.h"
#define FILE_ARRAY_SIZE 8
#define RTC_TYPE        0
#define DIRECTORY_TYPE  1
#define FILE_TYPE       2
#define TERMINAL_TYPE   3
#define USER_MAP_LOCATION 0x84b8000
#define USER_SPACE_START  0x8000000
#define USER_SPACE_END    0x8400000
#define STACK_FENCE    4


/* The fd structure*/
typedef struct file_entry_t
{
    fop_t* file_operations_table_pointer;       // pointer to the write, read, open, close function
    int inode;                                  // the file inode number
    int file_position;                          // file current position, 0 for terminal file and RTC file
    int flags;                                  // 1 for used and 0 for unused
}file_entry_t;

/* set the current file array */
void set_fd_array(file_entry_t* file_array_in);
/* fill the first two entry of the file array */
int32_t init_file_array(file_entry_t* file_array_in);
/* free all fd in the file array*/
void free_file_array(void);

/*10 sys calls */
// int32_t halt(uint8_t status);
// int32_t execute(const uint8_t* command);
int32_t read(int32_t fd, void* buf, int32_t nbytes);
int32_t write(int32_t fd, const void* buf, int32_t nbytes);
/* set the given file at the file array */
int32_t open(const uint8_t* filename);
/* remove the file from file array */
int32_t close(int32_t fd);
/* get the arg in PCB */
int32_t getargs(uint8_t* buf, int32_t nbytes);
/* get a pointer to video mem */
int32_t vidmap(uint8_t** screen_start);
/* set the handler of the signal */
int32_t set_handler(int32_t signum, void* handler_address);
/* return from sig handler to user space */
int32_t sigreturn(void);
file_entry_t* get_file_array();

/* system call -- execute, used to create new process */
int32_t execute (const uint8_t* command);

/* system call -- halt, used to terminal the current process */
int32_t halt (uint8_t status);


#endif
