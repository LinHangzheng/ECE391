/* fop.h - file defines for file operations table */
/* Created by Chenting on 10/28/2019 */




#ifndef _FOP_H
#define _FOP_H

#include "types.h"


#define RTC_IDX     0
#define DIR_IDX     1
#define FILE_IDX    2
#define TEMINAL_IDX 3
#define NUM_DRIVERS 4


/* structure for file operation table */
typedef struct fop_t{
    int32_t (*open)     (const uint8_t* filename);
    int32_t (*close)    (int32_t fd);
    int32_t (*read)     (int32_t fd,  uint32_t offset, void* buf, int32_t nbytes);
    int32_t (*write)    (int32_t fd, const void* buf, int32_t nbytes);
} fop_t;


/* functions used to get corresponding kind operation table */
fop_t get_terminal_fop(void);
fop_t get_RTC_fop(void);
fop_t get_file_fop(void);
fop_t get_dir_fop(void);
fop_t get_stdin_fop(void);
fop_t get_stdout_fop(void);

/* array for file operation tables */
fop_t fop_table[NUM_DRIVERS];


/* initialize file operation tables array */
int32_t init_fop_table(void);


#endif

