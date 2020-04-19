/* ternimal.h - operational functions of ternimal */
            /* Created by Chenting on 10/23/2019 */

#include "terminal.h"
#include "lib.h"
#include "page.h"
#include "process.h"


#define SUCCESS 0
#define FAIL    -1

terminal_t terminals[terminal_num];
int32_t cur_terminal = 0;


/* 
 *  terminal_open
 *  DESCRIPTION: nothing for now
 *                 
 *  INPUTS: nothing for now
 *  OUTPUTS: nothing
 *  RETURN VALUE: 0 for SUCCESS
*/
int32_t terminal_open (const uint8_t* filename){
    // do nothing
    return SUCCESS;
}
/* 
 *  terminal_close
 *  DESCRIPTION: nothing for now
 *                 
 *  INPUTS: nothing for now
 *  OUTPUTS: nothing
 *  RETURN VALUE: 0 for SUCCESS
*/
int32_t terminal_close (int32_t fd){

    /* clear the buf */
    keyboard_clean();
    return SUCCESS;

}

/* 
 *  terminal_read(int32_t fd, void* buf, int32_t nbytes)
 *  DESCRIPTION: just call keyboard_read
 *                 
 *  INPUTS: fd: file dir, not used for now
 *          const char* buf, the input buf
 *          int32_t length, length of the buf
 *  OUTPUTS: fill n Bytes in the buf
 *  RETURN VALUE: ret -- number of bytes read
 */
int32_t terminal_read(int32_t fd, uint32_t offset, void* buf, int32_t nbytes){
    if (buf==NULL){return FAIL;}
    if (nbytes < 0){return FAIL;}
    int32_t ret = keyboard_read(fd, offset, buf, nbytes);
    return ret;

}


/* 
 *  terminal_write(const char* buf, int32_t length)
 *  DESCRIPTION: write to the terminal with
 *              the contant in the buf of length
 *                 
 *  INPUTS: const char* buf, the input buf
 *          int32_t length, length of the buf
 *  OUTPUTS: write and display i=on the terminal
 *  RETURN VALUE: count -- number of bytes written 
 *                or -1 for fail
 */
int32_t terminal_write (int32_t fd, const void* input_buf, int32_t nbytes){
   
    /* change buffer pointer type to char* */
    const char* buf = input_buf;
    /* return -1 if buf is invaild */
    if(buf == NULL){
        return FAIL;
    }

    /* count # of chars put on screen */
    int32_t count;  

    /* write chars onto screen  */
    for(count = 0; count < nbytes; count++){
        putc(buf[count]);
    }
    /* return # of bytes written */
    return count;
}


/* 
 *  terminal_init()
 *  DESCRIPTION: init the terminal， called in kernel    
 *  INPUTS: noun
 *  OUTPUTS: change the content in the terminals array accordingly
 *  RETURN VALUE: should always return 0 for success
 */
int32_t terminal_init(){
    int i;
    for (i=0;i<terminal_num;i++){
        terminals[i].buf_pos = 0;
        terminals[i].stdin_enable = 0;
        terminals[i].cursor_x = 0;
        terminals[i].cursor_y = 0;
        terminals[i].next_terminal = &terminals[(i+1)%terminal_num];
        terminals[i].video_buff = VIDEO_MEM + (i+1)*FOURKB;
        terminals[i].vidmap = 0;
        terminals[i].tid = i;
        terminals[i].curr_pid = -1;
        terminals[i].curr_process_ptr = NULL;
        SET_PTE_PARAMS((terminals[i].video_buff) >> OFFSET_12,1);
    }

    /*flush the TLB*/
    asm volatile(
        "movl %%cr3,%%eax     ;"
        "movl %%eax,%%cr3     ;"

        : : : "eax", "cc" 
    );
    keyboard_buf = terminals[0].keyboard_buf;
    buf_pos = terminals[0].buf_pos;
    screen_x = &terminals[0].cursor_x;
    screen_y = &terminals[0].cursor_y;
    return 0;
}


/* 
 *  terminal_switch(int32_t new_terminal)
 *  DESCRIPTION: perform terminal switch, called when alt+ f123 is pressed 
 *  INPUTS: the idx of the new terminal
 *  OUTPUTS: change the content in the terminals array accordingly
 *          also set page for video memory, copy the content out of and into according buffer
 *  RETURN VALUE: should always return 0 for success
 */
int32_t terminal_switch(int32_t new_terminal){
    /* if swith to current, do noting */
    if (new_terminal == cur_terminal){
        // printf_on_curr_terminal("\n===============================================================================\n");
	    // printf_on_curr_terminal("|                               ALREADY IN                                    |\n"); 
        // printf_on_curr_terminal("|                              THIS TERMINAL                                  |\n"); 
	    // printf_on_curr_terminal("===============================================================================\n");
        return 0;
    }

    /* save and update current buffer ptr and cursor position */
    terminal_t* cur_terminal_ptr = &terminals[cur_terminal];
    terminal_t* new_termianl_ptr = &terminals[new_terminal];
    
    /* save current info into structure */
    cur_terminal_ptr->buf_pos = buf_pos;

    /* restore new info from structure */
    keyboard_buf = new_termianl_ptr->keyboard_buf;
    buf_pos = new_termianl_ptr->buf_pos;
    
    screen_x = &(new_termianl_ptr->cursor_x);
    screen_y = &(new_termianl_ptr->cursor_y);
    update_cursor(*screen_x,*screen_y);


    /* switch current video memory */
    video_mem_switch(cur_terminal);
    memcpy((void*)(cur_terminal_ptr->video_buff),(void*)VIDEO_MEM,Four_KB);
    memcpy((void*)VIDEO_MEM,(void*)(new_termianl_ptr->video_buff),Four_KB);
    
    
    cur_terminal = new_terminal;
    video_mem_switch(running_terminal->tid);

    
    return 0;
}

/* 
 *  video_mem_switch(int32_t new_terminal)
 *  DESCRIPTION: change the memory mapping for video mem 
 *              called when schedule need to change the running task and 
 *              by the terminal switch
 *  INPUTS: the idx of the new terminal
 *  OUTPUTS: also set page for video memor
 *  RETURN VALUE: should always return 0 for success
 */
int32_t video_mem_switch(int32_t new_terminal){
    /* switch the screen_x and screen_y */
    screen_x = &terminals[new_terminal].cursor_x;
    screen_y = &terminals[new_terminal].cursor_y;

    /* if new terminal is current viewing termianl */
    if (new_terminal == cur_terminal){
        /* Remap the linear video memory to physical video memory */
        page_table[VIDEO_MEM>>12].PB_addr = VIDEO_MEM>>12;
        /* change the user video memeory mapping */
        page_table_usermap[VIDEO_MEM>>OFFSET_12].present = terminals[new_terminal].vidmap;
        page_table_usermap[VIDEO_MEM>>OFFSET_12].PB_addr = VIDEO_MEM>>OFFSET_12;

    }else{
        /* Remap the linear video memory to physical video buffer */
        page_table[VIDEO_MEM>>12].PB_addr = terminals[new_terminal].video_buff>>12;
        /* change the user video memeory mapping */
        page_table_usermap[VIDEO_MEM>>OFFSET_12].present = terminals[new_terminal].vidmap;
        page_table_usermap[VIDEO_MEM>>OFFSET_12].PB_addr = terminals[new_terminal].video_buff>>OFFSET_12;
    }

    /*flush the TLB*/
    asm volatile(
        "movl %%cr3,%%eax     ;"
        "movl %%eax,%%cr3     ;"

        : : : "eax", "cc" 
    );
    return 0;
}


/* 
 *  get_cur_termianl()
 *  DESCRIPTION: Get the current viewing termianl ptr
 *  INPUTS: none
 *  OUTPUTS: current viewing termianl ptr
 */
terminal_t* get_cur_terminal(){
    return &terminals[cur_terminal];
}
