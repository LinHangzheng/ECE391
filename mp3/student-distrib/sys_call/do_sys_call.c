#include "do_sys_call.h"
#include "../process.h"
#define SUCCESS         0
#define FAIL            -1
#include "../terminal.h"
#include "../page.h"
#include  "../signal.h"
file_entry_t* file_array = NULL;

/*
 *   set_fd_array(file_entry_t* file_array_in)
 *  DESCRIPTION: Set the file array             
 *  INPUTS: file_array_t* file_array, 
 *          a ptr to the file array of current processes
 *  OUTPUTS: file array set
 *  RETURN VALUE: none
 */
void set_fd_array(file_entry_t* file_array_in){
    /* sanity check input */
    if(file_array_in == NULL ){
        printf("set fd with a NULL ptr!!!");
        return;
    }
    // keep the file array ptr in globle
    file_array = file_array_in;
}


/*
 *  void init_file_array(file_array_t* file_array)
 *  DESCRIPTION: ini the file array, fill the two entry
 *  of file array with std_in and std_out  
 *  called every time we set up the file array              
 *  INPUTS: file_array_t* file_array, 
 *          a ptr to the file array of current processes
 *  OUTPUTS: file array initilized and filled with two entry
 *  RETURN VALUE: should always return 0 for SUCCESS
 */
int32_t init_file_array(file_entry_t* file_array_in){
    int i;
    /* sanity check input */
    if(file_array_in == NULL){
        printf("ini file with a NULL ptr!!!");
        return FAIL;
    }
    set_fd_array(file_array_in);
    // the first entry of the 
    file_array[0].file_operations_table_pointer = &fop_table[TEMINAL_IDX];
    file_array[0].inode = NULL;
    file_array[0].flags = 1;
    file_array[0].file_position = 0;

    // the second entry 
    file_array[1].file_operations_table_pointer= &fop_table[TEMINAL_IDX];
    file_array[1].inode = NULL;
    file_array[1].flags = 1;
    file_array[1].file_position = 0;

    //other:
    for(i = 2; i < FILE_ARRAY_SIZE; i++){
        file_array[i].flags = 0;
    }
    //done
    return SUCCESS;
}

/*
 *  int32_t open (const uint8_t* filename)
 *  DESCRIPTION: ini the file array, fill the one free entry
 *  of file array with the file                
 *  INPUTS: the file name
 *  OUTPUTS: file array filled with entry
 *  RETURN VALUE: should return fd for SUCCESS, -1 for fail
 */
int32_t open (const uint8_t* filename){
    int fd=-1;
    int i;
    dentry_t cur_dentry;

    /* if the filename is NULL fail */
    if (filename == NULL){
        printf("read NULL!");
        return FAIL;
    }
    if( (int)filename < USER_SPACE_START || (int)filename > USER_SPACE_END -4 ){return FAIL;}
    /* select the table that is not in use */
    for(i = 0; i < FILE_ARRAY_SIZE; i++ ){
        if(file_array[i].flags == 0){
            fd =i;
            break;
        }
    }
    /* if all is in use, return fail*/
    if(fd == -1){
        printf("open too many files!");
        return FAIL;
    }

    /*fail if not exist*/
    if( -1 == read_dentry_by_name(filename,&cur_dentry) ){
        printf("no such file");
        return FAIL; 
    };

    /* set up entry value in corresponding fd */
    file_array[fd].file_operations_table_pointer = &fop_table[cur_dentry.file_type];
    file_array[fd].file_position = 0;
    file_array[fd].flags = 1;
    /* if it is rtc, set it to -1 */
    if (cur_dentry.file_type == RTC_TYPE){
        file_array[fd].inode = -1;
    }else{file_array[fd].inode = cur_dentry.inode_idx;}
    
    /* if corresponding action fails, we shoudl fail too */
    if(FAIL == file_array[fd].file_operations_table_pointer->open(filename)){
        // file_array[fd].flags = 0;    
        return FAIL;
    }
    /* return fd for success */
    return fd;
}

/*
 *  int32_t close (const uint8_t* filename)
 *  DESCRIPTION: ini the file array, fill the one free entry
 *  of file array with the file                
 *  INPUTS: the file name
 *  OUTPUTS: file array filled with entry
 *  RETURN VALUE: should return fd for SUCCESS, -1 for fail
 */
int32_t close (int32_t fd){
    /* not valid idx, try to close default, or originally closed, fail */ 
    if(fd <= 1 || fd>=FILE_ARRAY_SIZE || file_array[fd].flags == 0){
        return FAIL;
    }
    /* close and call corresponding close */
    file_array[fd].flags = 0;
    return file_array[fd].file_operations_table_pointer->close(fd);
}


/*
 *  int32_t read(int32_t fd, void* buf, int32_t nbytes)
 *  DESCRIPTION: call the read function based on the file type             
 *  INPUTS: int32_t fd  - the file descriptor
 *          void* buf  - the dest buffer
 *          int32_t nbytes - the length of the read bytes
 *  OUTPUTS: based on the file type
 *  RETURN VALUE: should return fd for SUCCESS, -1 for fail
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes){
    /* if the fd is not in range 0~7 or 1, or the flags is 0,
     * or doesn't contain read funcion, return fail
     */
    int32_t ret;
    /* judge whether the ptr is in user space */
    if( (int)buf < USER_SPACE_START || (int)buf + nbytes > USER_SPACE_END -4 ){return FAIL;}
    if (nbytes <= 0){return FAIL;}
    if (fd < 0 || fd >= FILE_ARRAY_SIZE || fd == 1){return FAIL;}
    if (!file_array[fd].flags){return FAIL;}
    if (file_array[fd].file_operations_table_pointer->read == NULL){return FAIL;}



    /* Call the corresponding read function base on the file type */
    ret = (file_array[fd].file_operations_table_pointer->read)(fd,file_array[fd].file_position,buf,nbytes);
    return ret;
}


/*
 *  write(int32_t fd, const void* buf, int32_t nbytes)
 *  DESCRIPTION: call the write function based on the file type            
 *  INPUTS: int32_t fd  - the file descriptor
 *          void* buf  - the dest buffer
 *          int32_t nbytes - the length of the write bytes
 *  OUTPUTS: based on the file type
 *  RETURN VALUE: should return fd for SUCCESS, -1 for fail
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes){
    /* if the fd is not in range 1~7, or the flags is 0,
     * or doesn't contain read funcion, return fail
     */
    if (fd<=0 || fd >= FILE_ARRAY_SIZE){return FAIL;}
    if (!file_array[fd].flags || buf == NULL){return FAIL;}
    if (file_array[fd].file_operations_table_pointer->write == NULL){return FAIL;}

    /* judge whether the ptr is in user space */
    if( (int)buf < USER_SPACE_START || (int)buf + nbytes > USER_SPACE_END -4 ){return FAIL;}

    /* Call the corresponding write function base on the file type */
    return ((file_array[fd].file_operations_table_pointer->write)(fd,buf,nbytes));
}

/*
 *  void free_file_array(void)
 *  DESCRIPTION: free all fd in the file array           
 *  INPUTS: void
 *  OUTPUTS: free the file array
 *  RETURN VALUE: none
 */
void free_file_array(void){
    int fd;
    for (fd = 2; fd<FILE_ARRAY_SIZE; fd++){
        close(fd);
    }
    file_array[0].flags = 0;
    file_array[0].file_operations_table_pointer->close(0);
    file_array[1].flags = 0;
    file_array[1].file_operations_table_pointer->close(1);
}

/*
 *  int32_t getargs(uint8_t* buf, int32_t nbytes)
 *  DESCRIPTION: get the arg in PCB       
 *  INPUTS: buf, the arg should go to this buf
 *          nbytes: the length of the buf
 *  OUTPUTS: put arg into buf
 *  RETURN VALUE: 0 for success and -1 for fail
 */

int32_t getargs(uint8_t* buf, int32_t nbytes){

    /* judge whether the ptr is in user space */
    if( (int)buf < USER_SPACE_START || (int)buf + nbytes > USER_SPACE_END -4 ){return FAIL;}
    
    PCB_t* pcb = get_pcb();
    /*If no args, return Fail*/
    if (pcb->args[0]== '\0'){return FAIL;}
    /*copy the args to the user space*/
    strncpy((int8_t*)buf,pcb->args,nbytes);         
    return SUCCESS;
}


/*
 *  int32_t getargs(uint8_t* buf, int32_t nbytes)
 *  DESCRIPTION: get start of the video mem     
 *  INPUTS: screen_start, the place of the pointer to go to
 *  OUTPUTS: set a new page to video mem and put ptr into buf
 *  RETURN VALUE: 0 for success and -1 for fail
 */
int32_t vidmap(uint8_t** screen_start){

    /* judge whether the ptr is in user space */
    if( (int)screen_start < USER_SPACE_START || (int)screen_start > USER_SPACE_END -STACK_FENCE ){
        return FAIL;
    }

    PCB_t* pcb = get_pcb(); 
    /*Sanity Check*/
    if (screen_start == NULL){return FAIL;}

    /*Check if the pointer is fall within user space*/
    if ((int32_t)screen_start>>OFFSET_22 < USER_MEM_PDE_INDEX ||
        (int32_t)screen_start>>OFFSET_22 >= (USER_MEM_PDE_INDEX+1)){return FAIL;}

    /*open the page*/
    if(running_terminal->tid == get_cur_terminal()->tid ){
        SET_USER_VIDEO_MAP(VIDEO>>OFFSET_12,1);
    }else{
        SET_USER_VIDEO_MAP(running_terminal->video_buff >>OFFSET_12,1);
    }
    
    /*flush the TLB*/
    asm volatile(
        "movl %%cr3,%%eax     ;"
        "movl %%eax,%%cr3     ;"

        : : : "eax", "cc" 
    );

    pcb->vidmap = 1;
    running_terminal->vidmap = 1;

    *screen_start = (uint8_t*)(USER_MAP_LOCATION);
    return SUCCESS;
}

/* not used yet */
int32_t set_handler(int32_t signum, void* handler_address){
    return FAIL;
}

/* not used yet */
int32_t sigreturn(void){
    return FAIL;
}

/*
 *  file_entry_t* get_file_array()
 *  DESCRIPTION: get pointer of current fd array 
 *  INPUTS: none
 *  OUTPUTS: return the current fd array pointer
 */
file_entry_t* get_file_array(){
    return file_array;
}


/* system call -- execute, used to create new process */
int32_t execute (const uint8_t* command){
    return process_create(command);
}

/* system call -- halt, used to terminal the current process */
int32_t halt (uint8_t status){
    return process_terminate(status);
}
