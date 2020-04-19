#include "process.h"
#define FAIL -1
#define SUCCESS 0


uint8_t process_bitmap[MAX_PROCESSES]= {0,0,0,0,0,0};
int32_t curr_pid = -1;
int32_t new_pid = -1;
process_t* curr_process_ptr = NULL;


/* define constant for FAIL and SUCCESS */
#define FAIL -1
#define SUCCESS 0



/* process_create
 *  DESCRIPTION: do system call execute, used to create new process     
 *  INPUTS: const uint8_t* command -- pointer of command to be executed
 *  OUTPUTS: context switch to new processs
 *  RETURN VALUE: -1 for fail
 */
int32_t process_create (const uint8_t* command){
    /* Sanity check */
    cli();
    /* Sanity check */ 
    if (command == NULL){return FAIL;}

    /*parse the argument*/
    int8_t file_name[MAX_FILENAME_LEN+1];
    int8_t args[MAX_BUF];
    if (FAIL ==  _parse_argument((int8_t*)command, file_name,args)){return FAIL;}

    /* Check file validity */
    if(FAIL == _user_file_validity_check((uint8_t*)file_name)) return FAIL;

    /* get new pid for current process */
    int32_t pid = process_allocate();

    /* check whether process out of range */
    if(pid == -1){
        printf("====================================================\n");
        printf("|===---     CAN NOT HAVE MORE PROCESSES!     ---===|\n");
        printf("====================================================\n");
        return SUCCESS;
    }

    /* update current pid */
    new_pid = pid;

    /* Set up paging */
    _process_set_user_page(pid);

    /* Load file into memory */
    if(FAIL == _process_load_user_file((uint8_t*)file_name)) return FAIL;

    /* init PCB and corresponding file array */
    _init_pcb_and_file_array(args);

    /* Context switch */
    /* step when creating process 0 */
    if(curr_process_ptr->parent_pid != -1){
        /* get the pointer of old_PCB */
        int32_t old_pid = curr_process_ptr->parent_pid;
        PCB_t* old_PCB = get_process_ptr(old_pid);

        /* get current esp and ebp */
        uint32_t old_esp, old_ebp;
        asm volatile(
            "movl %%esp, %0 ;"
            "movl %%ebp, %1 ;"
            : "=r" (old_esp) ,"=r" (old_ebp) 
        );
        /* store esp and ebp value in PCB struct */
        old_PCB->esp = old_esp;
        old_PCB->ebp = old_ebp;
    }

    _context_switch_to_user();

    /* FAKE return */
    return SUCCESS;
}



/* process_terminate
 *  DESCRIPTION: do system call halt, used to terminal the current process       
 *  INPUTS: uint8_t status -- the status of halt need to be returned to user
 *  OUTPUTS: JMP to execute stack, then return to sys_call
 *  RETURN VALUE: retval -- return value corresponding to status
 */
int32_t process_terminate (uint8_t status){

    cli();
    /* free current process in bitmap */
    process_free(curr_pid);
    
    /* handle for root process */
    if(curr_process_ptr->parent_pid == -1){
        printf("====================================================\n");
        printf("|===---       CANNOT EXIT the ROOT SHELL     ---===|\n");
        printf("====================================================\n");
        curr_pid = -1;
        process_create((uint8_t*)"shell");
    }

    /* get pointer of parent process */
    process_t* parent_process_ptr = get_process_ptr(curr_process_ptr -> parent_pid);

    /* update esp0 in TSS */
    tss.esp0 = parent_process_ptr->tss_esp0;
    tss.ss0 = KERNEL_DS;

    /* Restore parent paging */
    _process_set_user_page(curr_process_ptr -> parent_pid);

    /* Close any relevant FDs */
    /* free all fd in the file array*/
    set_fd_array(curr_process_ptr->fd_array);
    free_file_array();

    /* set current fd array */
    set_fd_array(parent_process_ptr->fd_array);

    /* If the process have video memory, close it*/
    if (curr_process_ptr->vidmap){
        curr_process_ptr->vidmap = 0;
        running_terminal->vidmap = 0;
        if(running_terminal->tid == get_cur_terminal()->tid ){
            SET_USER_VIDEO_MAP(VIDEO>>OFFSET_12,0);
        }else{
            SET_USER_VIDEO_MAP(running_terminal->video_buff >>OFFSET_12,0);
        }
    }

    curr_pid = curr_process_ptr->parent_pid;
    curr_process_ptr = parent_process_ptr;
    int32_t old_esp = curr_process_ptr->esp;
    int32_t old_ebp = curr_process_ptr->ebp;
    /* update esp&ebp and return value */
    uint16_t retval = (uint16_t) status;
    if(status == EXCEPTION_STATUS) retval = EXCEPTION_RETVAL;
    asm volatile(
        "movl %0, %%esp ;"
        "movl %1, %%ebp ;"
        "xorl %%eax,%%eax;"
        "movw %2, %%ax ;"
        "leave;"
        "ret;"
        : 
        : "r" (old_esp), "r" (old_ebp), "r"(retval)
        : "esp", "ebp", "eax"
    );

    /* Jump to execute return */
    return SUCCESS;
}

/* _process_set_user_page
 *  
 *  DESCRIPTION: the function sets the page table for the program          
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: (fop_t) ret -- the operation table for ternimal operations
*/
void _process_set_user_page(uint32_t pid){

    /* determine the index in PD */
    uint32_t index = USER_MEM >> OFFSET_22;

    /* set the correpsonding 4MB page directory into use */
    page_directory[index].MB.present = 1;           //enable the page
    page_directory[index].MB.read_write = 1;        //read and write
    page_directory[index].MB.user_supervisor = 1;
    page_directory[index].MB.write_through = 0;
    page_directory[index].MB.cache_disabled = 0;
    page_directory[index].MB.accessed = 0;
    page_directory[index].MB.dirty = 0;
    page_directory[index].MB.page_size = 1;
    page_directory[index].MB.global_page = 0;
    page_directory[index].MB.availablr_for_user = 0;
    page_directory[index].MB.page_table_aindex = 0;
    page_directory[index].MB.reserved = 0;
    page_directory[index].MB.PB_addr = 2 + pid; //The starting physical address of mapping 

    /*flush the TLB*/
    asm volatile(
        "movl %%cr3,%%eax     ;"
        "movl %%eax,%%cr3     ;"

        : : : "eax", "cc" 
    );

}

/* _init_pcb_and_file_array
 *  DESCIPTION: helper function used to init the new pcb block and open file array
 *  INPUT: None
 *  OUTPUT: None
 *  RETURN: None
 */
void _init_pcb_and_file_array(int8_t* args){
    
    /* variables representing PCBs */
    process_t* new_process_ptr = get_process_ptr(new_pid);
    
    /* initial new PCB */
    new_process_ptr->pid = new_pid;
    new_process_ptr->parent_pid = curr_pid;
    new_process_ptr->vidmap = 0;

    /* init signal pending array */
    int32_t i = 0;
    for(i = 0; i < MAX_SIG_NUM; i++){
        new_process_ptr->sigpending[i] = 0;
    }

    /* init signal handlers */
    init_sighand(&new_process_ptr->sighand);

    /* update esp0 in TSS */
    uint32_t new_process_addr = (uint32_t) new_process_ptr;
    new_process_ptr->tss_esp0 = new_process_addr + KERNAL_STACK_SIZE - STACK_FENCE;

    /* initialize the fd array and rtc interrupt interval*/    
    init_file_array(new_process_ptr->fd_array);
    new_process_ptr->rtc_ival = 0;

    /*initialize the args*/
    strncpy(new_process_ptr->args,args,MAX_BUF);

    
    /* update global variables */
    curr_process_ptr = new_process_ptr;
    curr_pid = new_pid;

}

/* _user_file_validity_check
 *  DESCIPTION: helper function used to check file validity
 *  INPUT: const uint8_t* filename -- name of file to be checked
 *  OUTPUT: None
 *  RETURN: int32_t -- -1 for fail
 *                      0 for success
 */
int32_t _user_file_validity_check(const uint8_t* filename){

    /* space for file dentry */
    dentry_t file_dentry;

    /* read dentry by its name */
    if(FAIL == read_dentry_by_name(filename, &file_dentry)) return FAIL;

    /* check the magic number of execuable file */
    uint8_t buf[EXETUABLE_NUM_MAGIC_NUM];
    if(EXETUABLE_NUM_MAGIC_NUM != read_data (file_dentry.inode_idx, 0, buf, EXETUABLE_NUM_MAGIC_NUM) ) return FAIL;
    if( buf[0] != FIRST_MAGIC_NUM   || 
        buf[1] != SECOND_MAGIC_NUM  ||
        buf[2] != THIRD_MAGIC_NUM   ||
        buf[3] != FOURTH_MAGIC_NUM  ){
        /* return fail if magic numbers not match */
        return FAIL;
    }

    /* return success */
    return SUCCESS;

}


/* _process_load_user_file
 *  DESCIPTION: helper function used to load file into user memory
 *  INPUT: const uint8_t* filename -- name of file to be checked
 *  OUTPUT: corresponding file copy into memory from 0x08048000
 *  RETURN: int32_t -- -1 for fail
 *                      0 for success
 */
int32_t _process_load_user_file(const uint8_t* filename){
    
    /* space for file dentry */
    dentry_t file_dentry;

    /* read dentry by its name */
    if(FAIL == read_dentry_by_name(filename, &file_dentry)) return FAIL;

    /* get the current size of file */
    uint32_t file_length = get_file_size(file_dentry.inode_idx);
    
    /* counts and buffer used in memory copy */
    uint32_t curr_read_count = 0;
    uint32_t read_count = 0;
    uint8_t buf[BLOCK_SIZE_IN_BYTE];
    
    /* copy executable file into memory from 0x8048000 */
    do{
        curr_read_count = read_data(file_dentry.inode_idx, read_count, buf, BLOCK_SIZE_IN_BYTE);
        memcpy( (void*) MEM_COPY_BASE_ADDR + read_count, buf, curr_read_count);
        read_count += curr_read_count;
    }while(read_count != file_length);

    /* return success */
    return SUCCESS;
}


/* _context_switch_to_user
 *  DESCIPTION: helper function used to achirve context switch
 *  INPUT: None
 *  OUTPUT: Context switch to user program
 *  RETURN: None
 */
void _context_switch_to_user(void){

    /* update esp0 in TSS */
    tss.esp0 = curr_process_ptr->tss_esp0;
    tss.ss0 = KERNEL_DS;

    /* prepare info for context switch */
    uint32_t XSS = USER_DS;
    /* Leave Four byte for data allignment */
    uint32_t ESP = USER_MEM + USER_STACK_SIZE - STACK_FENCE;
    uint32_t XCS = USER_CS;
    uint8_t* entry_ptr = (uint8_t*) MEM_COPY_BASE_ADDR + EIP_OFFSET;
    uint32_t EIP = * (uint32_t*) entry_ptr;

    // printf("XSS: %x \nESP: %x \nXCS: %x \nEIP: %x \n", XSS, ESP, XCS, EIP);

    sti();

    /* do context switch */
    asm volatile(
        "movw  %%ax, %%ds;"
        "pushl %%eax;"
        "pushl %%ebx;"
        "pushfl  ;"
        "pushl %%ecx;"
        "pushl %%edx;"
        "IRET"
        :
        : "a"(XSS), "b"(ESP), "c"(XCS), "d"(EIP)
        : "cc", "memory"
    );

}


/* _parse_argument
 *  DESCIPTION: helper function used in execute to parse the argument into file name and args
 *  INPUT: command     --Pointer to the input command
 *         file_name   --Pointer to the parsed file_name
 *         args        --Pointer to the parsed args
 *  OUTPUT: Fill the parsed args and file_name array
 *  RETURN: 0 for Success, -1 for Fail
 */
int32_t _parse_argument(int8_t* command,int8_t* file_name, int8_t* args){
    /*sanity check*/
    if (command == NULL){return FAIL;}
    
    uint32_t read_pos = 0;                      //index for the input command
    uint32_t file_name_count = 0;               //index for the file_name
    uint32_t command_len = strlen(command);
    uint32_t arg_count = 0;                     //index for the args

    /*parse the file name*/
    /*delete the leading space*/
    while (command[read_pos] == ' '){read_pos++;}
    while (command[read_pos] != ' ' && read_pos < command_len)
    {   
        /*check for the file name length*/
        if (file_name_count > MAX_FILENAME_LEN){return FAIL;}
        file_name[file_name_count] = command[read_pos];
        read_pos++;
        file_name_count++;
    }
    file_name[file_name_count] = '\0';
    
    /*parse the args*/
    /*delete the leading space*/
    while (command[read_pos] == ' '){read_pos++;}
    while (read_pos < command_len)
    {   
        if (arg_count > MAX_BUF){return FAIL;}
        args[arg_count] = command[read_pos];
        read_pos++;
        arg_count++;
    }
    args[arg_count] = '\0';
    
    return SUCCESS;
}


/* get_pcb
 *  DESCIPTION: helper function to get the current PCB
 *  INPUT: None
 *  OUTPUT: The current PCB pointer
 */
PCB_t* get_pcb(void){
    return curr_process_ptr;
}

/*  get_process_ptr(int32_t pid)
 *  DESCIPTION: get the process based on the input pid
 *  INPUT: int32_t pid -- the process index
 *  OUTPUT: none
 *  RETURN: the process pointer based on the input pid
 */
process_t* get_process_ptr(int32_t pid){
    uint32_t process_pos = KERNAL_MEM_END - KERNAL_STACK_SIZE * (pid + 1);
    return (process_t*) process_pos;
}

/*  process_allocate(void)
 *  DESCIPTION: find an empty bitmap space and allocate the process
 *              to it
 *  INPUT: none
 *  OUTPUT: none
 *  RETURN: the allocated bitmap position for success,
 *          -1 for fail
 */
int32_t process_allocate(void){

    /* bitmap position */
    uint32_t bitmap_pos = 0;
    while(process_bitmap[bitmap_pos] == 1){
        bitmap_pos++;
        /* return -1 for no room */
        if(bitmap_pos == MAX_PROCESSES){
            return FAIL;
        }
    }

    /* return pid */
    process_bitmap[bitmap_pos] = 1;
    return bitmap_pos;
}

/*  process_free(int32_t pid)
 *  DESCIPTION: free the process in the bitmap based on its pid
 *  INPUT: int32_t pid - process index
 *  OUTPUT: none
 *  RETURN: the allocated bitmap position for success,
 *          -1 for fail
 */
int32_t process_free(int32_t pid){
    
    /* fail if double free */
    if(process_bitmap[pid] == 0){
        return FAIL;
    }

    /* return 0 for success */
    process_bitmap[pid] = 0;
    return SUCCESS;
    
}


/* the pointer of current-handled terminal */
terminal_t* running_terminal = terminals;


/*  process_switch(void)
 *  DESCIPTION: switch to the next avaliable process
 *  INPUT: none
 *  OUTPUT: change the current process
 *  RETURN: none
 */
void process_switch(void){

    /* store current pid and process pointer into structure */
    running_terminal->curr_pid = curr_pid;
    running_terminal->curr_process_ptr = (struct process_t*) curr_process_ptr;

    /* store the current stack info */
    if(curr_pid != -1){
        uint32_t curr_esp = 0;
        uint32_t curr_ebp = 0;
        asm volatile(
            "movl %%esp, %0 ;"
            "movl %%ebp, %1 ;"
            : "=r" (curr_esp) ,"=r" (curr_ebp) 
        );
        curr_process_ptr->esp = curr_esp;
        curr_process_ptr->ebp = curr_ebp;
        curr_process_ptr->tss_esp0 = tss.esp0;
    }

    /* get info of next terminal and process */
    terminal_t* next_terminal = running_terminal->next_terminal;
    process_t* next_process_ptr = (process_t*)next_terminal->curr_process_ptr;
    int32_t next_pid = next_terminal->curr_pid;

    /* update current-handled terminal */
    running_terminal = next_terminal;
    /* update current-handled process */
    curr_process_ptr = next_process_ptr;
    curr_pid = next_pid;
    /* switch to next process */
    if(curr_pid == -1){
        /* start new shell if no shell running */
        printf("===============================================================================\n"); 
        printf("|                 |     |     |     |                                         |\n"); 
        printf("| TERMINAL SWITCH |  1  |  2  |  3  |    TERMINAL #%d                          |\n", (running_terminal->tid)+1); 
        printf("|                 |     |     |     |                                         |\n"); 
	    printf("===============================================================================\n");
        process_create((uint8_t*) "shell");
    }else{

        /* reset pages of user space */
        _process_set_user_page(curr_pid);

        /* update stack info */
        uint32_t curr_esp = curr_process_ptr->esp;
        uint32_t curr_ebp = curr_process_ptr->ebp;
        asm volatile(
            "movl %0, %%esp ;"
            "movl %1, %%ebp ;"
            : 
            : "r" (curr_esp), "r" (curr_ebp)
            : "esp", "ebp"
        );
        tss.esp0 = curr_process_ptr->tss_esp0;
        tss.ss0 = KERNEL_DS; 
    }

    _process_set_user_page(curr_pid);
    /* set current fd array */
    set_fd_array(curr_process_ptr->fd_array);
    
    /* return */
    asm volatile(
        "leave  ;"
        "ret    ;"
    );
}

/* get_process_bitmap
 *  DESCIPTION: helper function to get the current process_bitmap
 *  INPUT: None
 *  OUTPUT: The process_bitmap pointer
 */
uint8_t* get_process_bitmap(){
    return process_bitmap;
}

