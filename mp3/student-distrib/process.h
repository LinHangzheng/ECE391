

#ifndef _TASK_H
#define _TAKK_H

#include "types.h"
#include "devices/fs.h"
#include "page.h"
#include "x86_desc.h"
#include "sys_call/do_sys_call.h"
#include "terminal.h"
#include "signal.h"

/* Maximum number of processes can be executed */
#define MAX_PROCESSES 6

/*Maximum number of terminal input*/
#define MAX_BUF 128

/* kernel and user info in virtual addresses */
#define USER_MEM            0x08000000
#define USER_PROCESS_ADDR   0x08048000
#define USER_MEM_END        0x08400000      //132 MB
#define KERNAL_STACK_SIZE   0x2000          //8KB memory for PCB
#define KERNAL_MEM_END      0x800000        //8MB end of kenal memory
#define USER_STACK_SIZE     0x400000        //4MB

/* constant used in offset */
#define OFFSET_22 22
#define USER_MEM_PDE_INDEX 32
#define EIP_OFFSET  24
#define STACK_FENCE 4

/* magic numbers info in execuable file */
#define EXETUABLE_NUM_MAGIC_NUM 40      /* the total number of magic numbers */
#define FIRST_MAGIC_NUM         0x7f    
#define SECOND_MAGIC_NUM        0x45
#define THIRD_MAGIC_NUM         0x4c
#define FOURTH_MAGIC_NUM        0x46   

/* base address for executable file copy */
#define MEM_COPY_BASE_ADDR      0x8048000

/* status used for exception detected */
#define EXCEPTION_STATUS        0x0F
#define EXCEPTION_RETVAL        256
#define COMMAND_SIZE            128
#define SPACE                   ' '

/* maximum number of signals */
#define MAX_SIG_NUM         5



/* structure of PCB */
typedef struct process_t {

    /* current process id */
    int32_t pid;       
    uint32_t vidmap;
    uint8_t cmd[COMMAND_SIZE+1];
    uint8_t arg[COMMAND_SIZE+1];

    /* the pointer to parrent PCB */ 
    int32_t parent_pid;
    volatile int RTC_interrupt_occur;
    /* corresponding fd array */
    file_entry_t fd_array[FILE_ARRAY_SIZE];
    int32_t rtc_ival;

    /* store the current pos on stack */
    uint32_t esp;
    uint32_t ebp;
    uint32_t tss_esp0;

    /* argument for the process */    
    int8_t args[MAX_BUF];

    /* argument for the signal */
    struct sighand_t sighand;
    int32_t sigpending[MAX_SIG_NUM];
    int32_t sigmask;

} process_t;

typedef struct process_t PCB_t;


/* system call -- execute, used to create new process */
int32_t process_create (const uint8_t* command);

/* system call -- halt, used to terminal the current process */
int32_t process_terminate (uint8_t status);

/* helper function used to check file validity */
int32_t _user_file_validity_check(const uint8_t* filename);

/* helper function used to load file into user memory */
int32_t _process_load_user_file(const uint8_t* filename);

/* helper function used to achirve context switch to user */
void _context_switch_to_user(void);

/* helper function used to enable the page for the new program */
void _process_set_user_page(uint32_t pid);

/* helper function used to init the new pcb block and open file array */
void _init_pcb_and_file_array(int8_t* args);

int32_t _parse_argument(int8_t* command,int8_t* file_name, int8_t* args);

/*helper function to get the current PCB*/
PCB_t* get_pcb(void);

process_t* get_process_ptr(int32_t pid);
/* find an unused process unmber */
int32_t process_allocate(void);
/* free the data structrue of the process, called in halt */
int32_t process_free(int32_t pid);
/* called in the handler of the PIT, perform process switch */
void process_switch(void);

extern terminal_t* running_terminal;
extern int32_t curr_pid;
extern process_t* curr_process_ptr;
extern uint8_t process_bitmap[6];

uint8_t* get_process_bitmap();

#endif
