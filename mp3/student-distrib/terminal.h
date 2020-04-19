/* ternimal.h - Implement for ternimal operations */
            /* Created by Chenting on 10/23/2019 */



#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "types.h"
#include "devices/keyboard.h"
#include "devices/cursor.h"


#define terminal_num 3
#define VIDEO_MEM       0xB8000
#define VIDEO_BUFFER_1  0xB9000
#define VIDEO_BUFFER_2  0xBA000
#define VIDEO_BUFFER_3  0XBB000
#define Four_KB             4096


typedef struct terminal_t{
    
    char keyboard_buf[MAX_BUF_SIZE];
    volatile uint8_t stdin_enable;
    uint8_t buf_pos;
    int cursor_x;
    int cursor_y;
    uint32_t video_buff;

    /* parameters used for multi-processes control */
    int32_t curr_pid;
    struct process_t* curr_process_ptr;
    
    struct terminal_t* next_terminal;
    int32_t tid;
    int32_t vidmap;
} terminal_t;

/* nothing for now */
int32_t terminal_open (const uint8_t* filename);
/* nothing for now */
int32_t terminal_close (int32_t fd);
/* just call keyboard_read */
int32_t terminal_read(int32_t fd, uint32_t offset, void* buf, int32_t nbytes);
/* write to the terminal with the contant in the buf of length */
int32_t terminal_write (int32_t fd, const void* input_buf, int32_t nbytes);

int32_t terminal_init(void);
/* swith the terminal called in keyboard interrupt */
int32_t terminal_switch(int32_t new_terminal);
/* set the page to the video memory or cache called in swith process */
int32_t video_mem_switch(int32_t new_terminal);

extern terminal_t terminals[terminal_num];
terminal_t* get_cur_terminal();

#endif

